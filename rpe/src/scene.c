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

rpe_scene_t* rpe_scene_init(rpe_engine_t* engine, arena_t* arena)
{
    rpe_scene_t* i = ARENA_MAKE_ZERO_STRUCT(arena, rpe_scene_t);
    i->draw_data = ARENA_MAKE_ARRAY(arena, struct DrawData, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    MAKE_DYN_ARRAY(rpe_object_t, arena, 100, &i->objects);

    // Setup the camera UBO and model SSBOs.
    i->draw_data_handle = vkapi_res_cache_create_ssbo(
        engine->driver->res_cache,
        engine->driver,
        sizeof(struct DrawData) * RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        0,
        VKAPI_BUFFER_HOST_TO_GPU);

    // Setup the culling compute shader.
    i->cull_compute = rpe_compute_init_from_file(engine->driver, "cull.comp.spv", arena);

    rpe_compute_bind_ubo_buffer(i->cull_compute, 0, engine->camera_ubo);
    i->scene_ubo = rpe_compute_bind_ubo(i->cull_compute, engine->driver, 1);

    // Extents buffer for frustum culling visibility checks.
    i->extents_buffer = rpe_compute_bind_ssbo_host_gpu(
        i->cull_compute, engine->driver, 0, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    // Initial indirect draw data - created on the CPU, updated into the indirect_draw buffers by
    // the compute shader.
    i->mesh_data_handle = rpe_compute_bind_ssbo_host_gpu(
        i->cull_compute, engine->driver, 1, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    // For colour pass draws.
    i->model_draw_data_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        engine->driver,
        2,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    i->indirect_draw_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        engine->driver,
        4,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    // For shadow draws.
    i->shadow_model_draw_data_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        engine->driver,
        3,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    i->shadow_indirect_draw_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        engine->driver,
        5,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

    // Batched draw counts buffer.
    i->draw_count_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        engine->driver,
        6,
        VKAPI_DRIVER_MAX_DRAW_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    // Shadow batched draw counts buffer.
    i->shadow_draw_count_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        engine->driver,
        7,
        VKAPI_DRIVER_MAX_DRAW_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    // Total draw counts for both colour pass and shadow. This is only used in the compute shader.
    i->total_draw_handle = rpe_compute_bind_ssbo_gpu_only(i->cull_compute, engine->driver, 8, 2, 0);

    i->render_queue = rpe_render_queue_init(arena);
    i->rend_extents =
        ARENA_MAKE_ZERO_ARRAY(arena, rpe_rend_extents_t, RPE_SCENE_MAX_STATIC_MODEL_COUNT);

    // This is required to stop a validation layer message regarding the fact that no release
    // has yet been done on the graphics queue, when applying the barrier to the compute queue.
    vkapi_cmdbuffer_t* cmds = vkapi_driver_get_gfx_cmds(engine->driver);
    vkapi_driver_release_buffer_barrier(
        engine->driver, cmds, i->indirect_draw_handle, VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ);
    vkapi_driver_release_buffer_barrier(
        engine->driver, cmds, i->draw_count_handle, VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ);
    vkapi_driver_flush_gfx_cmds(engine->driver);

    return i;
}

bool rpe_scene_update(rpe_scene_t* scene, rpe_engine_t* engine)
{
    assert(scene);
    assert(scene->curr_camera);

    rpe_rend_manager_t* rm = engine->rend_manager;
    rpe_transform_manager_t* tm = engine->transform_manager;
    vkapi_driver_t* driver = engine->driver;
    struct Settings settings = engine->settings;

    rpe_render_queue_clear(scene->render_queue);

    rpe_light_manager_update(engine->light_manager, scene, scene->curr_camera);
    rpe_shadow_manager_update(
        engine->shadow_manager, scene->curr_camera, scene, engine, engine->light_manager);

    // Prepare the camera frustum - update the camera matrices before constructing the frustum.
    rpe_frustum_t frustum;
    math_mat4f vp = math_mat4f_mul(scene->curr_camera->projection, scene->curr_camera->view);
    rpe_frustum_projection(&frustum, &vp);

    rpe_transform_manager_update_ssbo(tm);
    arena_dyn_array_t* batched_draws = rpe_rend_manager_batch_renderables(rm, &scene->objects);
    rpe_scene_upload_extents(scene, engine, rm, tm);

    arena_dyn_array_t indirect_draws;
    MAKE_DYN_ARRAY(struct IndirectDraw, &engine->frame_arena, 500, &indirect_draws);

    vkapi_cmdbuffer_t* cmds = vkapi_driver_get_compute_cmds(driver);

    // Ensure the indirect commands have all been committed before updating the compute shader.
    vkapi_driver_acquire_buffer_barrier(
        driver,
        cmds,
        engine->curr_scene->indirect_draw_handle,
        VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);
    vkapi_driver_acquire_buffer_barrier(
        driver,
        cmds,
        engine->curr_scene->draw_count_handle,
        VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);

    // Update renderable objects.
    for (size_t i = 0; i < batched_draws->size; ++i)
    {
        rpe_batch_renderable_t* batch = DYN_ARRAY_GET_PTR(rpe_batch_renderable_t, batched_draws, i);
        for (size_t j = batch->first_idx; j < batch->first_idx + batch->count; ++j)
        {
            rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, &scene->objects, j);
            if (rpe_comp_manager_has_obj(rm->comp_manager, *obj))
            {
                rpe_renderable_t* rend = rpe_rend_manager_get_mesh(rm, obj);

                struct IndirectDraw draw = {0};
                draw.indirect_cmd.firstIndex = rend->mesh_data->index_offset;
                draw.indirect_cmd.indexCount = rend->mesh_data->index_count;
                draw.indirect_cmd.vertexOffset = (int32_t)rend->mesh_data->vertex_offset;
                draw.object_id = rpe_comp_manager_get_obj_idx(
                    engine->transform_manager->comp_manager, rend->transform_obj);
                draw.batch_id = i;
                draw.shadow_caster = rend->material->shadow_caster;
                DYN_ARRAY_APPEND(&indirect_draws, &draw);

                // The draw data is the per-material instance - different texture samplers can be
                // used without having to re-bind descriptors as we are using bindless samplers.
                scene->draw_data[j] = rend->material->material_draw_data;
            }
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

            // 2. The actual indirect draw (indexed) command.
            rpe_cmd_packet_t* pkt1 = rpe_command_bucket_append_command(
                scene->render_queue->gbuffer_bucket,
                pkt0,
                0,
                sizeof(struct DrawIndirectIndexCommand),
                &engine->frame_arena,
                rpe_cmd_dispatch_draw_indirect_indexed);
            struct DrawIndirectIndexCommand* cmd = pkt1->cmds;
            cmd->stride = sizeof(struct IndirectDraw);
            cmd->count_handle = scene->draw_count_handle;
            cmd->draw_count_offset = 0; // < offset for each batch??
            cmd->cmd_handle = scene->indirect_draw_handle;
            cmd->offset = batch->first_idx * sizeof(struct IndirectDraw);
        }
    }

    rpe_shadow_manager_t* sm = engine->shadow_manager;

    // ==================== Depth pass =========================
    if (settings.draw_shadows)
    {
        // All visible shadow casters drawn with one draw call using the data generated via
        // the compute-shader (no batching required as the pipeline remains the same for all calls).
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
        cmd->draw_count_offset = 0; // < total draw buffer?
        cmd->cmd_handle = scene->shadow_indirect_draw_handle;
        cmd->offset = 0;
    }

    vkapi_driver_map_gpu_buffer(
        driver,
        scene->mesh_data_handle,
        scene->objects.size * sizeof(struct IndirectDraw),
        0,
        indirect_draws.data);

    // Update the camera and scene UBO.
    rpe_camera_ubo_t cam_ubo = rpe_camera_update_ubo(scene->curr_camera, &frustum);
    vkapi_driver_map_gpu_buffer(driver, engine->camera_ubo, sizeof(rpe_camera_ubo_t), 0, &cam_ubo);

    struct SceneUbo scene_ubo = {
        .model_count = scene->objects.size,
        .ibl_mip_levels = scene->curr_ibl ? scene->curr_ibl->options.specular_level_count : 0};
    vkapi_driver_map_gpu_buffer(
        engine->driver, scene->scene_ubo, sizeof(rpe_scene_ubo_t), 0, &scene_ubo);

    vkapi_driver_clear_gpu_buffer(driver, cmds, scene->draw_count_handle);
    vkapi_driver_clear_gpu_buffer(driver, cmds, scene->shadow_draw_count_handle);
    vkapi_driver_clear_gpu_buffer(driver, cmds, scene->total_draw_handle);

    // Update the renderable extents buffer on the GPU and dispatch the culling compute shader.
    vkapi_driver_dispatch_compute(
        driver, scene->cull_compute->bundle, scene->objects.size / 128 + 1, 1, 1);

    vkapi_driver_release_buffer_barrier(
        driver,
        cmds,
        engine->curr_scene->indirect_draw_handle,
        VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);
    vkapi_driver_release_buffer_barrier(
        engine->driver,
        cmds,
        engine->curr_scene->draw_count_handle,
        VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);

    // Update GPU SSBO draw data buffer.
    vkapi_driver_map_gpu_buffer(
        driver,
        scene->draw_data_handle,
        scene->objects.size * sizeof(struct DrawData),
        0,
        scene->draw_data);

    return true;
}

void rpe_scene_upload_extents(
    rpe_scene_t* scene, rpe_engine_t* engine, rpe_rend_manager_t* rm, rpe_transform_manager_t* tm)
{
    assert(scene);
    assert(engine);
    assert(rm);
    assert(tm);

    for (size_t i = 0; i < scene->objects.size; ++i)
    {
        rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, &scene->objects, i);
        if (rpe_comp_manager_has_obj(rm->comp_manager, *obj))
        {
            rpe_renderable_t* rend = rpe_rend_manager_get_mesh(rm, obj);
            rpe_transform_node_t* transform =
                rpe_transform_manager_get_node(tm, rend->transform_obj);

            rpe_rend_extents_t* t = &scene->rend_extents[i];
            math_mat4f model_world =
                math_mat4f_mul(transform->world_transform, transform->local_transform);

            rpe_aabox_t box = {.min = rend->box.min, .max = rend->box.max};
            rpe_aabox_t world_box = rpe_aabox_calc_rigid_transform(
                &box,
                math_mat4f_to_rotation_matrix(model_world),
                math_mat4f_translation_vec(model_world));
            t->extent = math_vec4f_init_vec3(rpe_aabox_get_half_extent(&world_box), 0.0f);
            t->center = math_vec4f_init_vec3(rpe_aabox_get_center(&world_box), 0.0f);
        }
    }

    vkapi_driver_map_gpu_buffer(
        engine->driver,
        scene->extents_buffer,
        scene->objects.size * sizeof(rpe_rend_extents_t),
        0,
        scene->rend_extents);
}

/** Public functions **/

void rpe_scene_set_current_camera(rpe_scene_t* scene, rpe_engine_t* engine, rpe_camera_t* cam)
{
    assert(scene);
    scene->curr_camera = cam;

    // Update the shadow cascade maps based upon the current camera near/far values.
    rpe_shadow_manager_compute_cascade_proj(engine->shadow_manager, cam);
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
