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
#include <getopt.h>
#include <gltf/gltf_asset.h>
#include <gltf/gltf_loader.h>
#include <gltf/resource_loader.h>
#include <rpe/material.h>
#include <rpe/renderable_manager.h>
#include <stdlib.h>
#include <utility/filesystem.h>

void print_usage()
{
    printf("Usage:\n");
    printf("gltf_viewer [OPTIONS] <GLTF_MODEL_PATH> \n");
    printf("--win_width\t Window width in pixels\n");
    printf("--win_height\t Window height in pixels\n");
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        print_usage();
        exit(0);
    }

    uint32_t win_width = 1920;
    uint32_t win_height = 1080;

    struct option options[] = {
        {"help", no_argument, NULL, 'h'},
        {"win-width", required_argument, NULL, 'w'},
        {"win-height", required_argument, NULL, 'e'},
        {0, 0, NULL, 0}};

    int opt_index = 0;
    int opt;

    while ((opt = getopt_long(argc, argv, "hw:h:", options, &opt_index)) != -1)
    {
        switch (opt)
        {
            case 'h':
                print_usage();
                exit(0);
            case 'w':
                win_width = strtol(optarg, NULL, 0);
                break;
            case 'e':
                win_height = strtol(optarg, NULL, 0);
                break;
            default:
                break;
        }
    }

    if (argv[optind] == NULL)
    {
        printf("Gltf model path not specified.\n");
        print_usage();
        exit(1);
    }
    const char* gltf_model_path = argv[optind];

    rpe_app_t app;
    int error = rpe_app_init("model loader", win_width, win_height, &app);
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

    rpe_rend_manager_t* rm = rpe_engine_get_rend_manager(app.engine);
    rpe_renderer_t* renderer = rpe_engine_create_renderer(app.engine);

    FILE* fp = fopen(gltf_model_path, "r");
    if (!fp)
    {
        log_error("Unable to open gltf model at path: %s", gltf_model_path);
        exit(1);
    }
    size_t file_sz = fs_get_file_size(fp);
    uint8_t* buffer = malloc(file_sz);
    fread(buffer, sizeof(uint8_t), file_sz, fp);

    gltf_asset_t* asset = gltf_model_parse_data(buffer, file_sz, app.engine, gltf_model_path);
    if (!asset)
    {
        exit(1);
    }

    gltf_resource_loader_load_textures(asset, app.engine);
    free(buffer);

    // Add objects created by gltf loader to the scene.
    // TODO: move this to the gltf loader.
    for (size_t i = 0; i < asset->objects.size; ++i)
    {
        rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, &asset->objects, i);
        if (rpe_rend_manager_has_obj(rm, obj))
        {
            rpe_scene_add_object(app.scene, *obj);
        }
    }

    rpe_app_run(&app, renderer, NULL, NULL);

    rpe_app_shutdown(&app);

    exit(0);
}
