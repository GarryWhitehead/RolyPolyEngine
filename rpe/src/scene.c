/* Copyright (c) 2024-2025 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "scene.h"

#include "camera.h"
#include "commands.h"
#include "compute.h"
#include "engine.h"
#include "frustum.h"
#include "ibl.h"
#include "managers/component_manager.h"
#include "managers/light_manager.h"
#include "managers/renderable_manager.h"
#include "managers/transform_manager.h"
#include "material.h"
#include "render_queue.h"
#include "shadow_manager.h"
#include "skybox.h"

#include <tracy/TracyC.h>
#include <utility/job_queue.h>
#include <utility/parallel_for.h>

rpe_scene_t* rpe_scene_init(rpe_engine_t* engine, arena_t* arena)
{
    vkapi_driver_t* driver = engine->driver;

    rpe_scene_t* i = ARENA_MAKE_ZERO_STRUCT(arena, rpe_scene_t);
    i->shadow_status = engine->settings.draw_shadows ? RPE_SCENE_SHADOW_STATUS_ENABLED
                                                     : RPE_SCENE_SHADOW_STATUS_DISABLED;
    i->draw_data = ARENA_MAKE_ARRAY(arena, struct DrawData, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    MAKE_DYN_ARRAY(rpe_object_t, arena, 100, &i->objects);

    // Setup the camera UBO and model SSBOs.
    i->draw_data_handle = vkapi_res_cache_create_ssbo(
        driver->res_cache,
        driver,
        sizeof(struct DrawData) * RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        0,
        VKAPI_BUFFER_HOST_TO_GPU);

    // Camera ubo for this scene.
    i->camera_ubo = vkapi_res_cache_create_ubo(driver->res_cache, driver, sizeof(rpe_camera_ubo_t));

    // Setup the culling compute shader.
    i->cull_compute = rpe_compute_init_from_file(driver, "cull.comp.spv", arena);

    rpe_compute_bind_ubo_buffer(i->cull_compute, 0, i->camera_ubo);
    i->scene_ubo = rpe_compute_bind_ubo(i->cull_compute, driver, 1);

    // Extents buffer for frustum culling visibility checks.
    i->extents_buffer = rpe_compute_bind_ssbo_host_gpu(
        i->cull_compute, driver, 0, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    // Initial indirect draw data - created on the CPU, updated into the indirect_draw buffers by
    // the compute shader.
    i->mesh_data_handle = rpe_compute_bind_ssbo_host_gpu(
        i->cull_compute, driver, 1, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    // For colour pass draws.
    i->model_draw_data_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        driver,
        2,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    i->indirect_draw_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        driver,
        4,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    // For shadow draws.
    i->shadow_model_draw_data_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        driver,
        3,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    i->shadow_indirect_draw_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        driver,
        5,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

    // Batched draw counts buffer.
    i->draw_count_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        driver,
        6,
        VKAPI_DRIVER_MAX_DRAW_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    // Shadow batched draw counts buffer.
    i->shadow_draw_count_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        driver,
        7,
        VKAPI_DRIVER_MAX_DRAW_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    // Total draw counts for both colour pass and shadow. This is only used in the compute shader.
    i->total_draw_handle = rpe_compute_bind_ssbo_gpu_only(i->cull_compute, driver, 8, 2, 0);

    i->render_queue = rpe_render_queue_init(arena);
    i->rend_extents =
        ARENA_MAKE_ZERO_ARRAY(arena, rpe_rend_extents_t, RPE_SCENE_MAX_STATIC_MODEL_COUNT);

    // This is required to stop a validation layer message regarding the fact that no release
    // has yet been done on the graphics queue, when applying the barrier to the compute queue.
    vkapi_cmdbuffer_t* cmds = vkapi_driver_get_gfx_cmds(driver);
    vkapi_driver_release_buffer_barrier(
        driver, cmds, i->indirect_draw_handle, VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ);
    vkapi_driver_release_buffer_barrier(
        engine->driver, cmds, i->draw_count_handle, VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ);
    vkapi_driver_flush_gfx_cmds(driver);

    MAKE_DYN_ARRAY(rpe_batch_renderable_t, arena, 100, &i->batched_draw_cache);

    return i;
}

bool rpe_scene_update(rpe_scene_t* scene, rpe_engine_t* engine)
{
    TracyCZoneN(ctx, "Scene::Update", 1);

    assert(scene);
    assert(scene->curr_camera);

    rpe_rend_manager_t* rm = engine->rend_manager;
    rpe_transform_manager_t* tm = engine->transform_manager;
    rpe_shadow_manager_t* sm = engine->shadow_manager;

    vkapi_driver_t* driver = engine->driver;
    struct Settings settings = engine->settings;
    bool draw_shadows =
        scene->shadow_status == RPE_SCENE_SHADOW_STATUS_ENABLED && settings.draw_shadows;

    rpe_render_queue_clear(scene->render_queue);

    rpe_light_manager_update(engine->light_manager, scene, scene->curr_camera);
    if (draw_shadows)
    {
        rpe_shadow_manager_update_projections(
            engine->shadow_manager, scene->curr_camera, scene, engine, engine->light_manager);
    }

    // Prepare the camera frustum - update the camera matrices before constructing the frustum.
    rpe_frustum_t frustum;
    math_mat4f vp = math_mat4f_mul(scene->curr_camera->projection, scene->curr_camera->view);
    rpe_frustum_projection(&frustum, &vp);

    rpe_transform_manager_update_ssbo(tm);

    arena_dyn_array_t renderables;
    MAKE_DYN_ARRAY(struct RenderableInstance, &engine->frame_arena, 200, &renderables);
    // Segregate the renderables, lights, etc. from the scenes objects into their own lists, this
    // is required mainly so we don't have races when running updates on multiple threads.
    for (size_t i = 0; i < scene->objects.size; ++i)
    {
        rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, &scene->objects, i);
        if (rpe_comp_manager_has_obj(rm->comp_manager, *obj))
        {
            rpe_renderable_t* r = rpe_rend_manager_get_mesh(rm, obj);
            rpe_transform_node_t* transform =
                rpe_transform_manager_get_node(tm, r->transform_obj);
            struct RenderableInstance instance = {.rend = r, .transform = transform};
            DYN_ARRAY_APPEND(&renderables, &instance);
        }
        // Check for lights, transforms here.
    }

    if (scene->is_dirty)
    {
        rpe_rend_manager_batch_renderables(
            rm, renderables.data, renderables.size, &scene->batched_draw_cache);
        scene->is_dirty = false;
    }

    job_t* parent = job_queue_create_parent_job(engine->job_queue);
    struct UploadExtentsEntry entry = {
        .scene = scene,
        .engine = engine,
        .rm = rm,
        .tm = tm,
        .instances = renderables.data,
        .count = renderables.size};
    rpe_scene_compute_model_extents(&entry, parent);

    arena_dyn_array_t indirect_draws;
    MAKE_DYN_ARRAY(struct IndirectDraw, &engine->frame_arena, 100, &indirect_draws);

    vkapi_cmdbuffer_t* cmds = vkapi_driver_get_compute_cmds(driver);

    // Ensure the indirect commands have all been committed before updating the compute shader.
    vkapi_driver_acquire_buffer_barrier(
        driver, cmds, scene->indirect_draw_handle, VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);
    vkapi_driver_acquire_buffer_barrier(
        driver, cmds, scene->draw_count_handle, VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);

    // Update renderable objects.
    arena_dyn_array_t* batched_draws = &scene->batched_draw_cache;

    for (size_t i = 0; i < batched_draws->size; ++i)
    {
        rpe_batch_renderable_t* batch = DYN_ARRAY_GET_PTR(rpe_batch_renderable_t, batched_draws, i);
        for (size_t j = batch->first_idx; j < batch->first_idx + batch->count; ++j)
        {
            struct RenderableInstance* instance = DYN_ARRAY_GET_PTR(struct RenderableInstance, &renderables, j);
            rpe_renderable_t* rend = instance->rend;

            struct IndirectDraw draw = {0};
            draw.indirect_cmd.firstIndex = rend->mesh_data->index_offset;
            draw.indirect_cmd.indexCount = rend->mesh_data->index_count;
            draw.indirect_cmd.vertexOffset = (int32_t)rend->mesh_data->vertex_offset;
            draw.object_id = rpe_comp_manager_get_obj_idx(tm->comp_manager, rend->transform_obj);
            draw.batch_id = i;
            draw.shadow_caster = rend->material->shadow_caster;
            draw.perform_cull_test = rend->perform_cull_test;
            DYN_ARRAY_APPEND(&indirect_draws, &draw);

            // The draw data is the per-material instance - different texture samplers can be
            // used without having to re-bind descriptors as we are using bindless samplers.
            scene->draw_data[j] = rend->material->material_draw_data;
            // These specialisation constants are set by the scene.
            rend->material->material_consts.has_lighting = !scene->skip_lighting_pass;
        }

        {
            // ==================== Colour GBuffer =========================
            // 1. Bind the graphics pipeline (along with descriptor sets).
            rpe_cmd_packet_t* pkt0 = rpe_command_bucket_add_command(
                scene->render_queue->gbuffer_bucket,
                0,
                sizeof(struct PipelineBindCommand),
                &engine->frame_arena,
                rpe_cmd_dispatch_pline_bind);
            struct PipelineBindCommand* pl_cmd = pkt0->cmds;
            pl_cmd->bundle = batch->material->program_bundle;

            // 2. Bind the scissor [optional]
            rpe_cmd_packet_t* nxt_pkt = pkt0;
            if (batch->scissor.width > 0)
            {
                rpe_cmd_packet_t* pkt1 = rpe_command_bucket_append_command(
                    scene->render_queue->gbuffer_bucket,
                    nxt_pkt,
                    0,
                    sizeof(struct ScissorCommand),
                    &engine->frame_arena,
                    rpe_cmd_dispatch_scissor_cmd);
                struct ScissorCommand* sc_cmd = pkt1->cmds;
                sc_cmd->scissor = batch->scissor;
                nxt_pkt = pkt1;
            }

            // 3. Bind the viewport [optional]
            if (batch->viewport.rect.width > 0)
            {
                rpe_cmd_packet_t* pkt2 = rpe_command_bucket_append_command(
                    scene->render_queue->gbuffer_bucket,
                    nxt_pkt,
                    0,
                    sizeof(struct ViewportCommand),
                    &engine->frame_arena,
                    rpe_cmd_dispatch_viewport_cmd);
                struct ViewportCommand* vp_cmd = pkt2->cmds;
                vp_cmd->vp = batch->viewport;
                nxt_pkt = pkt2;
            }

            // 4. The actual indirect draw (indexed) command.
            rpe_cmd_packet_t* pkt3 = rpe_command_bucket_append_command(
                scene->render_queue->gbuffer_bucket,
                nxt_pkt,
                0,
                sizeof(struct DrawIndirectIndexCommand),
                &engine->frame_arena,
                rpe_cmd_dispatch_draw_indirect_indexed);
            struct DrawIndirectIndexCommand* cmd = pkt3->cmds;
            cmd->stride = sizeof(struct IndirectDraw);
            cmd->count_handle = scene->draw_count_handle;
            cmd->draw_count_offset = i * sizeof(uint32_t);
            cmd->cmd_handle = scene->indirect_draw_handle;
            cmd->offset = batch->first_idx * sizeof(struct IndirectDraw);
        }

        // ==================== Depth pass =========================
        if (draw_shadows)
        {
            rpe_cmd_packet_t* pkt0 = rpe_command_bucket_add_command(
                scene->render_queue->depth_bucket,
                0,
                sizeof(struct PipelineBindCommand),
                &engine->frame_arena,
                rpe_cmd_dispatch_pline_bind);
            struct PipelineBindCommand* pl_cmd = pkt0->cmds;
            pl_cmd->bundle = sm->csm_bundle;

            rpe_cmd_packet_t* pkt1 = rpe_command_bucket_append_command(
                scene->render_queue->depth_bucket,
                pkt0,
                0,
                sizeof(struct DrawIndirectIndexCommand),
                &engine->frame_arena,
                rpe_cmd_dispatch_draw_indirect_indexed);
            struct DrawIndirectIndexCommand* cmd = pkt1->cmds;
            cmd->stride = sizeof(struct IndirectDraw);
            cmd->count_handle = scene->shadow_draw_count_handle;
            cmd->draw_count_offset = i * sizeof(uint32_t);
            cmd->cmd_handle = scene->shadow_indirect_draw_handle;
            cmd->offset = 0;
        }
    }

    vkapi_driver_map_gpu_buffer(
        driver,
        scene->mesh_data_handle,
        scene->objects.size * sizeof(struct IndirectDraw),
        0,
        indirect_draws.data);

    // Update the camera and scene UBO.
    rpe_camera_ubo_t cam_ubo = rpe_camera_update_ubo(scene->curr_camera, &frustum);
    vkapi_driver_map_gpu_buffer(driver, scene->camera_ubo, sizeof(rpe_camera_ubo_t), 0, &cam_ubo);

    struct SceneUbo scene_ubo = {
        .model_count = scene->objects.size,
        .ibl_mip_levels = scene->curr_ibl ? scene->curr_ibl->options.specular_level_count : 0};
    vkapi_driver_map_gpu_buffer(
        engine->driver, scene->scene_ubo, sizeof(rpe_scene_ubo_t), 0, &scene_ubo);

    vkapi_driver_clear_gpu_buffer(driver, cmds, scene->draw_count_handle);
    vkapi_driver_clear_gpu_buffer(driver, cmds, scene->shadow_draw_count_handle);
    vkapi_driver_clear_gpu_buffer(driver, cmds, scene->total_draw_handle);

    // Ensure the model extent jobs are complete and data uploaded to the device before executing
    // the compute.
    rpe_scene_sync_and_upload_extents(scene, engine, renderables.size, parent);

    // Update the renderable extents buffer on the GPU and dispatch the culling compute shader.
    vkapi_driver_dispatch_compute(
        driver, scene->cull_compute->bundle, scene->objects.size / 128 + 1, 1, 1);

    vkapi_driver_release_buffer_barrier(
        driver, cmds, scene->indirect_draw_handle, VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);
    vkapi_driver_release_buffer_barrier(
        engine->driver, cmds, scene->draw_count_handle, VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);

    // Update GPU SSBO draw data buffer.
    vkapi_driver_map_gpu_buffer(
        driver,
        scene->draw_data_handle,
        scene->objects.size * sizeof(struct DrawData),
        0,
        scene->draw_data);

    TracyCZoneEnd(ctx);

    return true;
}

void rpe_scene_compute_model_extents_runner(uint32_t start, uint32_t count, void* data)
{
    assert(data);
    struct UploadExtentsEntry* entry = (struct UploadExtentsEntry*)data;
    assert(entry->scene);
    assert(entry->engine);
    assert(entry->rm);
    assert(entry->tm);

    for (size_t i = start; i < start + count; ++i)
    {
        struct RenderableInstance instance = entry->instances[i];
        rpe_renderable_t* rend = instance.rend;

        if (!rend->perform_cull_test)
        {
            continue;
        }

        rpe_rend_extents_t* t = &entry->scene->rend_extents[i];
        math_mat4f model_world = instance.transform->world_transform;

        rpe_aabox_t box = {.min = rend->box.min, .max = rend->box.max};
        rpe_aabox_t world_box = rpe_aabox_calc_rigid_transform(
            &box,
            math_mat4f_to_rotation_matrix(model_world),
            math_mat4f_translation_vec(model_world));
        t->extent = math_vec4f_init_vec3(rpe_aabox_get_half_extent(&world_box), 0.0f);
        t->center = math_vec4f_init_vec3(rpe_aabox_get_center(&world_box), 0.0f);
    }
}

void rpe_scene_compute_model_extents(struct UploadExtentsEntry* entry, job_t* parent)
{
    size_t count = entry->count;
    struct SplitConfig cfg = {.min_count = 64, .max_split = 12};
    job_t* job = parallel_for(
        entry->engine->job_queue,
        parent,
        0,
        count,
        rpe_scene_compute_model_extents_runner,
        entry,
        &cfg,
        &entry->engine->scratch_arena);
    job_queue_run_job(entry->engine->job_queue, job);
}

void rpe_scene_sync_and_upload_extents(
    rpe_scene_t* scene, rpe_engine_t* engine, size_t renderable_count, job_t* parent)
{
    // Ensure all model extent jobs have finished running before uploading.
    job_queue_run_and_wait(engine->job_queue, parent);
    arena_reset(&engine->scratch_arena);

    vkapi_driver_map_gpu_buffer(
        engine->driver,
        scene->extents_buffer,
        renderable_count * sizeof(rpe_rend_extents_t),
        0,
        scene->rend_extents);
}

/** Public functions **/

void rpe_scene_set_current_camera(rpe_scene_t* scene, rpe_engine_t* engine, rpe_camera_t* cam)
{
    assert(scene);
    scene->curr_camera = cam;

    // Update the shadow cascade maps based upon the current camera near/far values.
    rpe_shadow_manager_compute_csm_splits(engine->shadow_manager, scene, cam);
}

void rpe_scene_set_ibl(rpe_scene_t* scene, ibl_t* ibl)
{
    assert(scene);
    scene->curr_ibl = ibl;
}

void rpe_scene_add_object(rpe_scene_t* scene, rpe_object_t obj)
{
    assert(scene);
    DYN_ARRAY_APPEND(&scene->objects, &obj);
    scene->is_dirty = true;
}

bool rpe_scene_remove_object(rpe_scene_t* scene, rpe_object_t obj)
{
    assert(scene);
    for (size_t i = 0; i < scene->objects.size; ++i)
    {
        rpe_object_t* other = DYN_ARRAY_GET_PTR(rpe_object_t, &scene->objects, i);
        if (obj.id == other->id)
        {
            DYN_ARRAY_REMOVE(&scene->objects, i);
            scene->is_dirty = true;
            return true;
        }
    }
    return false;
}

void rpe_scene_set_current_skyox(rpe_scene_t* scene, rpe_skybox_t* sb)
{
    assert(scene);
    assert(sb);
    assert(vkapi_tex_handle_is_valid(sb->cube_texture));

    // Avoid duplicated skybox requests as this will lead to the skybox being drawn multiple times.
    if (scene->curr_skybox && scene->curr_skybox == sb)
    {
        return;
    }
    scene->curr_skybox = sb;
    rpe_scene_add_object(scene, sb->obj);
}

void rpe_scene_set_shadow_status(rpe_scene_t* scene, enum ShadowStatus status)
{
    assert(scene);
    scene->shadow_status = status;
}

void rpe_scene_skip_lighting_pass(rpe_scene_t* scene)
{
    assert(scene);
    scene->skip_lighting_pass = true;
}
