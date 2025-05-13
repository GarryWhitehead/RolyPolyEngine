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

#include <app/app.h>
#include <rpe/material.h>
#include <rpe/object.h>
#include <rpe/object_manager.h>
#include <rpe/renderable_manager.h>
#include <rpe/settings.h>
#include <rpe/transform_manager.h>

#define MODELS_PER_AXIS 5

int main()
{
    const uint32_t win_width = 1920;
    const uint32_t win_height = 1080;

    rpe_app_t app;
    struct Settings settings = {.gbuffer_dims = 2048, .draw_shadows = false};
    int error = rpe_app_init("HelloTriangles", win_width, win_height, &app, &settings, false);
    if (error != APP_SUCCESS)
    {
        exit(1);
    }

    swapchain_handle_t* sc =
        rpe_engine_create_swapchain(app.engine, app.window.vk_surface, win_width, win_height);
    if (!sc)
    {
        exit(1);
    }
    rpe_engine_set_current_swapchain(app.engine, sc);

    rpe_renderer_t* renderer = rpe_engine_create_renderer(app.engine);
    rpe_rend_manager_t* r_manager = rpe_engine_get_rend_manager(app.engine);
    rpe_obj_manager_t* o_manager = rpe_engine_get_obj_manager(app.engine);
    rpe_transform_manager_t* t_manager = rpe_engine_get_transform_manager(app.engine);

    // Create the triangle mesh data.
    rpe_material_t* mat = rpe_rend_manager_create_material(r_manager, app.scene);
    rpe_material_set_cull_mode(mat, RPE_CULL_MODE_NONE);
    rpe_material_set_test_enable(mat, true);
    rpe_material_set_write_enable(mat, true);
    rpe_material_set_depth_compare_op(mat, RPE_COMPARE_OP_LESS);
    rpe_material_set_front_face(mat, RPE_FRONT_FACE_COUNTER_CLOCKWISE);

    math_vec3f pos_vertices[3] = {{1.0f, -1.0f, 0.0f}, {-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    math_vec4f col_vertices[3] = {
        {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};
    int32_t indices[] = {0, 1, 2};

    rpe_valloc_handle vbuffer_handle = rpe_rend_manager_alloc_vertex_buffer(r_manager, 3);
    rpe_valloc_handle ibuffer_handle = rpe_rend_manager_alloc_index_buffer(r_manager, 3);

    rpe_mesh_t* mesh = rpe_rend_manager_create_static_mesh(
        r_manager,
        vbuffer_handle,
        (float*)pos_vertices,
        NULL,
        NULL,
        (float*)col_vertices,
        3,
        ibuffer_handle,
        indices,
        3,
        RPE_RENDERABLE_INDICES_U32);

    rpe_renderable_t* renderable = rpe_engine_create_renderable(app.engine, mat, mesh);

    for (int z = 0; z < MODELS_PER_AXIS; ++z)
    {
        for (int y = 0; y < MODELS_PER_AXIS; ++y)
        {
            for (int x = 0; x < MODELS_PER_AXIS; ++x)
            {
                rpe_object_t obj = rpe_obj_manager_create_obj(o_manager);
                rpe_object_t t_obj = rpe_obj_manager_create_obj(o_manager);
                rpe_rend_manager_add(r_manager, renderable, obj, t_obj);
                rpe_scene_add_object(app.scene, obj);

                rpe_model_transform_t t = rpe_model_transform_init();
                t.scale = math_vec3f_init(0.2f, 0.2f, 0.2f);
                t.translation = math_vec3f_init((float)x / 2.0f, (float)y / 2.0f, (float)z / 2.0f);
                rpe_transform_manager_add_local_transform(t_manager, &t, &t_obj);
            }
        }
    }

    rpe_camera_view_set_position(&app.window.cam_view, math_vec3f_init(0.0f, 0.0f, -4.0f));
    rpe_app_run(&app, renderer, NULL, NULL, NULL, NULL, NULL);

    rpe_app_shutdown(&app);

    exit(0);
}
