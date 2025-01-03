/* Copyright (c) 2024 Garry Whitehead
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
#include "managers/component_manager.h"
#include "managers/renderable_manager.h"
#include "managers/transform_manager.h"
#include "material.h"
#include "render_queue.h"

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
    i->cam_ubo = rpe_compute_bind_ubo(i->cull_compute, engine->driver, 0);
    i->scene_ubo = rpe_compute_bind_ubo(i->cull_compute, engine->driver, 1);
    i->extents_buffer = rpe_compute_bind_ssbo_host_gpu(
        i->cull_compute, engine->driver, 0, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    i->mesh_data_handle = rpe_compute_bind_ssbo_host_gpu(
        i->cull_compute, engine->driver, 1, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    i->model_draw_data_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        engine->driver,
        2,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    i->indirect_draw_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        engine->driver,
        3,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    i->draw_count_handle = rpe_compute_bind_ssbo_gpu_only(
        i->cull_compute,
        engine->driver,
        4,
        VKAPI_DRIVER_MAX_DRAW_COUNT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    i->total_draw_handle = rpe_compute_bind_ssbo_gpu_only(i->cull_compute, engine->driver, 5, 1, 0);

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

    rpe_render_queue_clear(scene->render_queue);

    // Prepare the camera frustum - update the camera matrices before constructing the frustum.
    rpe_frustum_t frustum;
    math_mat4f vp = math_mat4f_mul(scene->curr_camera->projection, scene->curr_camera->view);
    rpe_frustum_projection(&frustum, &vp);

    // TODO
    math_mat4f world_transform = math_mat4f_identity();

    rpe_transform_manager_update_ssbo(engine->transform_manager);
    arena_dyn_array_t* batched_draws =
        rpe_rend_manager_batch_renderables(engine->rend_manager, &scene->objects);
    rpe_scene_upload_extents(
        scene, engine, engine->rend_manager, engine->transform_manager, world_transform);

    arena_dyn_array_t indirect_draws;
    MAKE_DYN_ARRAY(struct IndirectDraw, &engine->frame_arena, 1000, &indirect_draws);

    vkapi_cmdbuffer_t* cmds = vkapi_driver_get_compute_cmds(engine->driver);

    // Ensure the indirect commands have all been committed before updating the compute shader.
    vkapi_driver_acquire_buffer_barrier(
        engine->driver,
        cmds,
        engine->curr_scene->indirect_draw_handle,
        VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);
    vkapi_driver_acquire_buffer_barrier(
        engine->driver,
        cmds,
        engine->curr_scene->draw_count_handle,
        VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE);

    // Update renderable objects.
    for (size_t i = 0; i < batched_draws->size; ++i)
    {
        rpe_batch_renderable_t* batch = DYN_ARRAY_GET_PTR(rpe_batch_renderable_t, batched_draws, i);
        for (size_t j = batch->first_idx; j < batch->count; ++j)
        {
            rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, &scene->objects, j);
            rpe_renderable_t* rend = rpe_rend_manager_get_mesh(engine->rend_manager, obj);

            struct IndirectDraw draw = {0};
            draw.indirect_cmd.firstIndex = rend->mesh_data->index_offset;
            draw.indirect_cmd.indexCount = rend->mesh_data->index_count;
            draw.indirect_cmd.vertexOffset = (int32_t)rend->mesh_data->vertex_offset;
            draw.object_id =
                rpe_comp_manager_get_obj_idx(engine->transform_manager->comp_manager, *obj);
            draw.batch_id = i;
            DYN_ARRAY_APPEND(&indirect_draws, &draw);

            // The draw data is the per-material instance - different texture samplers can be used
            // without having to re-bind descriptors as we are using bindless samplers.
            scene->draw_data[j] = rend->material->material_draw_data;
        }

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
        cmd->draw_count_offset = 0;
        cmd->cmd_handle = scene->indirect_draw_handle;
        cmd->offset = 0;
    }

    vkapi_driver_map_gpu_buffer(
        engine->driver,
        scene->mesh_data_handle,
        scene->objects.size * sizeof(struct IndirectDraw),
        0,
        indirect_draws.data);

    // Update the camera and scene UBO.
    rpe_camera_ubo_t cam_ubo = rpe_camera_update_ubo(scene->curr_camera, &frustum);
    vkapi_driver_map_gpu_buffer(
        engine->driver, scene->cam_ubo, sizeof(rpe_camera_ubo_t), 0, &cam_ubo);
    struct SceneUbo scene_ubo = {.model_count = scene->objects.size};
    vkapi_driver_map_gpu_buffer(
        engine->driver, scene->scene_ubo, sizeof(rpe_scene_ubo_t), 0, &scene_ubo);

    vkapi_driver_clear_gpu_buffer(engine->driver, cmds, scene->draw_count_handle);
    vkapi_driver_clear_gpu_buffer(engine->driver, cmds, scene->total_draw_handle);

    // Update the renderable extents buffer on the GPU and dispatch the culling compute shader.
    vkapi_driver_dispatch_compute(
        engine->driver, scene->cull_compute->bundle, scene->objects.size / 128 + 1, 1, 1);

    vkapi_driver_release_buffer_barrier(
        engine->driver,
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
        engine->driver,
        scene->draw_data_handle,
        scene->objects.size * sizeof(struct DrawData),
        0,
        scene->draw_data);

    return true;
}

void rpe_scene_upload_extents(
    rpe_scene_t* scene,
    rpe_engine_t* engine,
    rpe_rend_manager_t* rm,
    rpe_transform_manager_t* tm,
    math_mat4f world_transform)
{
    assert(scene);
    assert(engine);
    assert(rm);
    assert(tm);

    for (size_t i = 0; i < scene->objects.size; ++i)
    {
        rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, &scene->objects, i);
        rpe_renderable_t* rend = rpe_rend_manager_get_mesh(rm, obj);
        rpe_transform_params_t* transform = rpe_transform_manager_get_transform(tm, *obj);

        rpe_rend_extents_t* t = &scene->rend_extents[i];
        math_mat4f model_world = math_mat4f_mul(world_transform, transform->model_transform);

        rpe_aabox_t box = {.min = rend->box.min, .max = rend->box.max};
        rpe_aabox_t world_box = rpe_aabox_calc_rigid_transform(
            &box,
            math_mat4f_to_rotation_matrix(model_world),
            math_mat4f_translation_vec(model_world));
        t->extent = math_vec4f_init_vec3(rpe_aabox_get_half_extent(&world_box), 0.0f);
        t->center = math_vec4f_init_vec3(rpe_aabox_get_center(&world_box), 0.0f);
    }

    vkapi_driver_map_gpu_buffer(
        engine->driver,
        scene->extents_buffer,
        scene->objects.size * sizeof(rpe_rend_extents_t),
        0,
        scene->rend_extents);
}

/** Public functions **/

rpe_scene_t* rpe_scene_create(rpe_engine_t* engine)
{
    return rpe_scene_init(engine, &engine->perm_arena);
}

void rpe_scene_set_current_camera(rpe_scene_t* scene, rpe_camera_t* cam)
{
    assert(scene);
    scene->curr_camera = cam;
}

void rpe_scene_add_object(rpe_scene_t* scene, rpe_object_t obj)
{
    assert(scene);
    DYN_ARRAY_APPEND(&scene->objects, &obj);
}
