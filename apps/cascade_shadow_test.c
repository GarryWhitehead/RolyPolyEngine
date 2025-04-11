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
#include <app/ibl_helper.h>
#include <getopt.h>
#include <gltf/gltf_asset.h>
#include <gltf/gltf_loader.h>
#include <gltf/resource_loader.h>
#include <rpe/ibl.h>
#include <rpe/light_manager.h>
#include <rpe/material.h>
#include <rpe/object_manager.h>
#include <rpe/renderable_manager.h>
#include <rpe/settings.h>
#include <rpe/skybox.h>
#include <rpe/transform_manager.h>
#include <stdlib.h>
#include <string.h>
#include <utility/filesystem.h>

#define MODEL_TREE_COUNT 5

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

void add_model_obj_to_scene(rpe_app_t* app, gltf_asset_t* asset, rpe_rend_manager_t* rm)
{
    for (size_t i = 0; i < asset->objects.size; ++i)
    {
        rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, &asset->objects, i);
        if (rpe_rend_manager_has_obj(rm, obj))
        {
            rpe_scene_add_object(app->scene, *obj);
        }
    }
}

int main(int argc, char** argv)
{
    const uint32_t win_width = 1920;
    const uint32_t win_height = 1080;

    struct option options[] = {{"help", no_argument, NULL, 'h'}, {0, 0, NULL, 0}};

    int opt_index = 0;
    int opt;

    while ((opt = getopt_long(argc, argv, "h", options, &opt_index)) != -1)
    {
        switch (opt)
        {
            case 'h':
                print_usage();
                exit(0);
            default:
                break;
        }
    }

    const char* gltf_asset_path = argv[optind];
    if (!gltf_asset_path)
    {
        log_error("No Git gltf asset directory specified.");
        print_usage();
        exit(1);
    }

    rpe_settings_t settings = {
        .gbuffer_dims = 2048,
        .draw_shadows = true,
        .shadow.cascade_dims = 1024,
        .shadow.split_lambda = 0.9f,
        .shadow.cascade_count = 4,
        .shadow.enable_debug_cascade = true};

    rpe_app_t app;
    int error = rpe_app_init("Cascade Shadow Demo", win_width, win_height, &app, &settings);
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

    // Add a directional light for shadows.
    rpe_object_t dir_obj = rpe_obj_manager_create_obj(om);
    float angle = math_to_radians(0.2f * 360.0f);
    float radius = 20.0f;
    rpe_light_create_info_t ci = {.position = cosf(angle), -radius, sinf(angle) * radius};
    rpe_light_manager_create_light(lm, &ci, dir_obj, RPE_LIGHTING_TYPE_DIRECTIONAL);

    const char* model_filenames[2] = {"/models/terrain_gridlines.gltf", "/models/oaktree.gltf"};
    gltf_asset_t* model_assets[2];

    math_vec3f tree_positions[MODEL_TREE_COUNT] = {
        {0.0f, 0.0f, 0.0f},
        {1.25f, 0.0f, 1.25f},
        {-1.25f, 0.0f, -0.2f},
        {1.25f, 0.0f, 0.1f},
        {-1.25f, 0.0f, -1.25f},
    };

    char full_path[4096] = {0};
    for (int i = 0; i < 2; ++i)
    {
        strcpy(full_path, gltf_asset_path);
        strcat(full_path, model_filenames[i]);

        FILE* fp = fopen(full_path, "r");
        if (!fp)
        {
            log_error("Unable to open gltf model at path: %s", full_path);
            exit(1);
        }
        size_t file_sz = fs_get_file_size(fp);
        uint8_t* buffer = malloc(file_sz);
        fread(buffer, sizeof(uint8_t), file_sz, fp);

        model_assets[i] = gltf_model_parse_data(buffer, file_sz, app.engine, full_path);
        if (!model_assets[i])
        {
            exit(1);
        }

        gltf_resource_loader_load_textures(model_assets[i], app.engine);
        free(buffer);
    }

    // We don't want the terrian to cast shadows.
    rpe_material_t* mat = DYN_ARRAY_GET(rpe_material_t*, &model_assets[0]->materials, 0);
    rpe_material_set_shadow_caster_state(mat, false);

    // Add the terrain model to the scene.
    add_model_obj_to_scene(&app, model_assets[0], rm);

    // Add trees to the scene.
    gltf_model_create_instances(
        model_assets[1], rm, tm, om, app.scene, MODEL_TREE_COUNT, tree_positions);

    rpe_app_run(&app, renderer, NULL, NULL);

    rpe_app_shutdown(&app);

    exit(0);
}