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
#include <app/nk_helper.h>
#include <gltf/gltf_asset.h>
#include <gltf/gltf_loader.h>
#include <gltf/resource_loader.h>
#include <parg.h>
#include <rpe/light_manager.h>
#include <rpe/material.h>
#include <rpe/object_manager.h>
#include <rpe/renderable_manager.h>
#include <rpe/settings.h>
#include <rpe/shadow_manager.h>
#include <rpe/skybox.h>
#include <rpe/transform_manager.h>
#include <stdlib.h>
#include <string.h>
#include <utility/filesystem.h>


#define MODEL_TREE_COUNT 10
#define MODEL_COUNT 1

/**
 * A app for testing the casccade shadows method. Draws a basic landscape scene using models
 * from the Vulkan Asset git repo. This must be cloned and a path passed via the cmd arg when
 * running this.
 */
void print_usage()
{
    printf("Usage:\n");
    printf("gltf_viewer [OPTIONS] <GLTF_ASSET_GIT_FOLDER> \n");
}

void create_ground_plane(rpe_engine_t* engine, rpe_scene_t* scene)
{
    uint32_t indices[] = {0, 1, 2, 2, 3, 0};

    const math_vec3f vertices[4] = {
        {-20.0f, 0.0f, -20.0f},
        {-20.0f, 0.0f, 20.0f},
        {20.0f, 0.0f, 20.0f},
        {20.0f, 0.0f, -20.0f},
    };

    const math_vec4f col = {0.0f, 0.7f, 0.1f, 1.0f};
    const math_vec4f colours[] = {col, col, col, col};

    rpe_rend_manager_t* rm = rpe_engine_get_rend_manager(engine);
    rpe_obj_manager_t* om = rpe_engine_get_obj_manager(engine);
    rpe_transform_manager_t* tm = rpe_engine_get_transform_manager(engine);

    rpe_valloc_handle vbuffer_handle = rpe_rend_manager_alloc_vertex_buffer(rm, 4);
    rpe_valloc_handle ibuffer_handle = rpe_rend_manager_alloc_index_buffer(rm, 6);

    rpe_mesh_t* mesh = rpe_rend_manager_create_static_mesh(
        rm,
        vbuffer_handle,
        (float*)vertices,
        NULL,
        NULL,
        (float*)colours,
        4,
        ibuffer_handle,
        indices,
        6,
        RPE_RENDERABLE_INDICES_U32);

    rpe_material_t* mat = rpe_rend_manager_create_material(rm, scene);
    rpe_material_set_cull_mode(mat, RPE_CULL_MODE_BACK);
    rpe_material_set_test_enable(mat, true);
    rpe_material_set_write_enable(mat, true);
    rpe_material_set_depth_compare_op(mat, RPE_COMPARE_OP_LESS);
    rpe_material_set_front_face(mat, RPE_FRONT_FACE_CLOCKWISE);
    rpe_material_set_shadow_caster_state(mat, false);

    rpe_renderable_t* renderable = rpe_engine_create_renderable(engine, mat, mesh);
    rpe_object_t obj = rpe_obj_manager_create_obj(om);
    rpe_object_t t_obj = rpe_obj_manager_create_obj(om);
    rpe_rend_manager_add(rm, renderable, obj, t_obj);
    rpe_scene_add_object(scene, obj);

    rpe_model_transform_t t = rpe_model_transform_init();
    t.translation = math_vec3f_init(10.0f, 0.0f, 10.0f);
    t.rot = math_mat4f_axis_rotate(math_to_radians(180.0f), math_vec3f_init(1.0f, 0.0f, 0.0f));
    rpe_transform_manager_add_local_transform(tm, &t, &t_obj);
}

struct LightData
{
    float timer_speed;
    float timer;
    rpe_object_t dir_obj;
};

void light_update(rpe_engine_t* engine, void* data)
{
    struct LightData* light_data = (struct LightData*)data;
    light_data->timer += light_data->timer_speed;
    if (light_data->timer > 1.0f)
    {
        light_data->timer = -1.0f;
    }

    float angle = math_to_radians(light_data->timer * 360.0f);
    float radius = 20.0f;
    math_vec3f pos = {cosf(angle), -radius, sinf(angle) * radius};
    rpe_light_manager_set_position(rpe_engine_get_light_manager(engine), light_data->dir_obj, &pos);
}

void ui_callback(rpe_engine_t* engine, rpe_scene_t* scene, app_window_t* win)
{
    nk_instance_t* nk = win->nk;
    struct nk_context* ctx = &nk->ctx;
    rpe_settings_t settings = rpe_engine_get_settings(engine);

    if (nk_begin(
            ctx,
            "Shadow Cascade Test",
            nk_rect(50, 50, 230, 250),
            NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE |
                NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 30, 1);
        if (nk_checkbox_label(ctx, "Draw shadows", (nk_bool*)&settings.draw_shadows))
        {
            rpe_engine_update_settings(engine, &settings);
        }

        // Cascade levels.
        nk_layout_row_dynamic(ctx, 30, 2);
        nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
        {
            nk_layout_row_push(ctx, 50);
            nk_label(ctx, "Cascade levels:", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 150);
            if (nk_slider_int(ctx, 1, &settings.shadow.cascade_count, 8, 1))
            {
                rpe_engine_update_settings(engine, &settings);
            }
        }

        // Lambda
        nk_layout_row_dynamic(ctx, 30, 2);
        nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
        {
            nk_layout_row_push(ctx, 50);
            nk_label(ctx, "Split lambda:", NK_TEXT_LEFT);
            nk_layout_row_push(ctx, 150);
            if (nk_slider_float(ctx, 0.1f, &settings.shadow.split_lambda, 1.0f, 0.1f))
            {
                rpe_engine_update_settings(engine, &settings);
            }
        }
    }
    nk_end(ctx);
}

int main(int argc, char** argv)
{
    const uint32_t win_width = 1920;
    const uint32_t win_height = 1080;
    const char* gltf_asset_path = NULL;
    bool show_ui = true;

    struct parg_state ps;
    parg_init(&ps);

    int opt;
    while ((opt = parg_getopt(&ps, argc, argv, "hd")) != -1)
    {
        switch (opt)
        {
            case 1:
                gltf_asset_path = ps.optarg;
                break;
            case 'h':
                print_usage();
                exit(0);
            case 'd':
                show_ui = false;
                break;
            default:
                break;
        }
    }

    if (!gltf_asset_path)
    {
        log_error("No Git gltf asset directory specified.");
        print_usage();
        exit(1);
    }

    rpe_settings_t settings = {
        .gbuffer_dims = 2048,
        .draw_shadows = true,
        .shadow.cascade_dims = 2048,
        .shadow.split_lambda = 0.9f,
        .shadow.cascade_count = 3,
        .shadow.enable_debug_cascade = false};

    rpe_app_t app;
    int error =
        rpe_app_init("Cascade Shadow Demo", win_width, win_height, &app, &settings, show_ui);
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

    rpe_obj_manager_t* om = rpe_engine_get_obj_manager(app.engine);
    rpe_rend_manager_t* rm = rpe_engine_get_rend_manager(app.engine);
    rpe_light_manager_t* lm = rpe_engine_get_light_manager(app.engine);
    rpe_transform_manager_t* tm = rpe_engine_get_transform_manager(app.engine);
    rpe_renderer_t* renderer = rpe_engine_create_renderer(app.engine);

    const char* model_filenames[MODEL_COUNT] = {"/models/oaktree.gltf"};
    gltf_asset_t* model_assets[MODEL_COUNT];

    math_vec3f tree_positions[MODEL_TREE_COUNT] = {
        {0.0f, 0.0f, 0.0f},
        {1.25f, 0.0f, 1.25f},
        {-3.25f, 0.0f, -0.2f},
        {3.25f, 0.0f, 2.1f},
        {-5.25f, 0.0f, -1.25f},
        {2.0f, 0.0f, -5.5f},
        {3.5f, 0.0f, -6.8f},
        {7.5f, 0.0f, -8.0f},
        {9.0f, 0.0f, 8.0f},
        {-5.0f, 0.0f, -5.0f}};

    char full_path[4096] = {0};
    for (int i = 0; i < MODEL_COUNT; ++i)
    {
        strcpy(full_path, gltf_asset_path);
        strcat(full_path, model_filenames[i]);

        FILE* fp = fopen(full_path, "rb");
        if (!fp)
        {
            log_error("Unable to open gltf model at path: %s", full_path);
            exit(1);
        }
        size_t file_sz = fs_get_file_size(fp);
        uint8_t* buffer = malloc(file_sz);
        size_t rd = fread(buffer, sizeof(uint8_t), file_sz, fp);
        assert(rd == file_sz);

        model_assets[i] = gltf_model_parse_data(buffer, file_sz, app.engine, full_path, &app.arena);
        if (!model_assets[i])
        {
            exit(1);
        }

        gltf_resource_loader_load_textures(model_assets[i], app.engine, &app.arena);
        free(buffer);
    }

    create_ground_plane(app.engine, app.scene);

    // Add trees to the scene.
    rpe_model_transform_t tree_transforms[MODEL_TREE_COUNT];
    for (int i = 0; i < MODEL_TREE_COUNT; ++i)
    {
        tree_transforms[i] = rpe_model_transform_init();
        tree_transforms[i].translation = tree_positions[i];
    }

    gltf_model_create_instances(
        model_assets[0], rm, tm, om, app.scene, MODEL_TREE_COUNT, tree_transforms, &app.arena);

    struct LightData data = {.timer = 0.2f, .timer_speed = 0.001f};

    // Add a directional light for shadows - just creating the object here, will be updated when
    // running the app.
    data.dir_obj = rpe_obj_manager_create_obj(om);
    rpe_light_create_info_t ci = {0};
    rpe_light_manager_create_light(lm, &ci, data.dir_obj, RPE_LIGHTING_TYPE_DIRECTIONAL);

    rpe_camera_view_set_position(&app.window.cam_view, math_vec3f_init(0.0f, -1.5f, -2.0f));
    rpe_app_run(&app, renderer, light_update, &data, NULL, NULL, ui_callback);

    rpe_app_shutdown(&app);

    exit(0);
}