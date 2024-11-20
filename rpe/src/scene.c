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

rpe_scene_t* rpe_scene_init(rpe_engine_t* engine, arena_t* arena)
{
    rpe_scene_t* i = ARENA_MAKE_ZERO_STRUCT(arena, rpe_scene_t);
    i->static_transforms = ARENA_MAKE_ARRAY(arena, math_mat4f, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    i->skinned_transforms =
        ARENA_MAKE_ARRAY(arena, math_mat4f, RPE_SCENE_MAX_SKINNED_MODEL_COUNT, 0);

    // Camera UBO.
    i->cam_ubo = vkapi_res_cache_create_ubo(
        engine->driver->res_cache,
        engine->driver,
        sizeof(struct CameraUbo),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        arena);

    // Setup the culling compute shader.
    i->cull_compute = rpe_compute_init_from_file(engine->driver, "cull.comp", arena);
    i->cam_ubo = rpe_compute_bind_ubo(i->cull_compute, engine->driver, 0);
    i->extents_buffer = rpe_compute_bind_ssbo(
        i->cull_compute,
        engine->driver,
        0,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT * sizeof(struct RenderableExtents));
    i->vis_status_buffer = rpe_compute_bind_ssbo(
        i->cull_compute, engine->driver, 1, RPE_SCENE_MAX_STATIC_MODEL_COUNT * sizeof(int));

    return i;
}

bool rpe_scene_update(rpe_scene_t* scene, rpe_engine_t* engine)
{
    rpe_obj_manager_t* om = &engine->obj_manager;

    rpe_render_queue_clear(&scene->render_queue);

    // Prepare the camera frustum
    // Update the camera matrices before constructing the fustrum
    rpe_frustum_t frustum;
    math_mat4f vp = math_mat4f_mul(&scene->curr_camera->projection, &scene->curr_camera->view);
    rpe_frustum_projection(&frustum, &vp);

    // TODO
    math_mat4f world_transform = math_mat4f_identity();

    size_t static_model_count = 0;
    size_t skinned_model_count = 0;

    // Update renderable objects.
    for (size_t i = 0; i < scene->objects.size; ++i)
    {
        rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, &scene->objects, i);

        rpe_obj_handle_t h = rpe_comp_manager_get_obj_idx(engine->rend_manager->comp_manager, *obj);
        if (!rpe_obj_handle_is_valid(h))
        {
            continue;
        }

        rpe_renderable_t* rend = rpe_rend_manager_get_mesh(engine->rend_manager, obj);
        rpe_transform_params_t* transform =
            rpe_transform_manager_get_transform(engine->transform_manager, *obj);

        uint64_t flags = rpe_renderable_get_primitive(rend, 0)->material_flags;
        if (flags & RPE_RENDERABLE_PRIM_HAS_SKIN)
        {
            ++skinned_model_count;
        }
        ++static_model_count;

        // Let's update the material now as all data that requires an update
        // "should" have been done by now for this frame.
        for (size_t j = 0; j < rend->primitives.size; ++j)
        {
            rpe_render_primitive_t* prim =
                DYN_ARRAY_GET_PTR(rpe_render_primitive_t, &rend->primitives, i);
            rpe_material_t* m = prim->material;

            // 1. Bind the graphics pipeline (along with descriptor sets).
            rpe_cmd_packet_t* pkt0 = rpe_command_bucket_add_command(
                scene->render_queue.gbuffer_bucket,
                m->sort_key,
                0,
                sizeof(struct PipelineBindCommand),
                &engine->frame_arena,
                rpe_cmd_dispatch_pline_bind);
            struct PipelineBindCommand* pl_cmd = pkt0->cmds;
            pl_cmd->bundle = prim->material->program_bundle;

            // 2. Conditional rendering setup. Visibility checks done on compute,
            // draw command isn't called if visibility check fails for an object.
            rpe_cmd_packet_t* pkt1 = rpe_command_bucket_append_command(
                scene->render_queue.gbuffer_bucket,
                pkt0,
                0,
                sizeof(struct CondRenderCommand),
                &engine->frame_arena,
                rpe_cmd_dispatch_cond_render);
            struct CondRenderCommand* cond_cmd = pkt1->cmds;
            cond_cmd->handle = scene->vis_status_buffer;
            cond_cmd->offset = 0; // TODO

            // 3. Push constant - index into the draw data mega buffer for this material.
            rpe_cmd_packet_t* pkt2 = rpe_command_bucket_append_command(
                scene->render_queue.gbuffer_bucket,
                pkt1,
                sizeof(struct MaterialPushData),
                sizeof(struct PushConstantCommand),
                &engine->frame_arena,
                rpe_cmd_dispatch_push_constant);
            struct PushConstantCommand* push_cmd = pkt2->cmds;
            push_cmd->size = sizeof(struct MaterialPushData);
            push_cmd->stage = RPE_BACKEND_SHADER_STAGE_FRAGMENT;
            push_cmd->data = pkt2->data;
            struct MaterialPushData push_data = {.draw_idx = m->handle.id};
            memcpy(pkt2->data, &push_data, sizeof(struct MaterialPushData));

            // 4. The actual draw (indexed) command.
            rpe_cmd_packet_t* pkt3 = rpe_command_bucket_append_command(
                scene->render_queue.gbuffer_bucket,
                pkt2,
                0,
                sizeof(struct DrawIndexCommand),
                &engine->frame_arena,
                rpe_cmd_dispatch_index_draw);
            struct DrawIndexCommand* cmd = pkt3->cmds;
            cmd->index_count = prim->index_count;
            cmd->index_offset = prim->index_offset;
            cmd->vertex_offset = prim->vertex_offset;
        }

        // Create the static and skinned transform buffers for visible models.
        scene->static_transforms[i] = transform->model_transform;
        if (transform->joint_matrices.size > 0)
        {
            // TODO: This needs checking if it actually works.
            memcpy(
                scene->skinned_transforms + skinned_model_count,
                transform->joint_matrices.data,
                sizeof(math_mat4f) * transform->joint_matrices.size);
            skinned_model_count += transform->joint_matrices.size;
        }

        // Update the buffer used for visibility checks on the GPU.
        rpe_rend_extents_t* t = &scene->rend_extents[i];
        math_mat4f model_world = math_mat4f_mul(&world_transform, &transform->model_transform);

        rpe_render_primitive_t* prim =
            DYN_ARRAY_GET_PTR(rpe_render_primitive_t, &rend->primitives, 0);
        rpe_aabox_t box = {.min = prim->box.min, .max = prim->box.max};
        rpe_aabox_t world_box = rpe_aabox_calc_rigid_transform(&box, &model_world);
        t->extent = rpe_aabox_get_half_extent(&world_box);
        t->center = rpe_aabox_get_center(&world_box);
    }

    // Update the renderable extents buffer on the GPU and dispatch the culling compute shader.
    vkapi_driver_map_gpu_buffer(
        engine->driver,
        scene->extents_buffer,
        static_model_count * sizeof(rpe_rend_extents_t),
        0,
        scene->rend_extents);
    vkapi_driver_dispatch_compute(
        engine->driver, scene->cull_compute->bundle, static_model_count / 16, 1, 1);

    // Update GPU SSBO transform buffers.
    if (static_model_count > 0)
    {
        vkapi_driver_map_gpu_buffer(
            engine->driver,
            scene->static_buffer_handle,
            static_model_count * sizeof(math_mat4f),
            0,
            scene->static_transforms->data);
    }
    if (skinned_model_count > 0)
    {
        vkapi_driver_map_gpu_buffer(
            engine->driver,
            scene->skinned_buffer_handle,
            skinned_model_count * sizeof(math_mat4f),
            0,
            scene->skinned_transforms->data);
    }

    // ================== update ubos =================================
    // sceneUbo_->updateCamera(*camera_);
    // sceneUbo_->updateIbl(indirectLight_);
    // sceneUbo_->updateDirLight(engine_, lm->getDirLightParams());
    // sceneUbo_->upload(engine_);


    // lm->updateSsbo(candLightObjs);

    return true;
}
