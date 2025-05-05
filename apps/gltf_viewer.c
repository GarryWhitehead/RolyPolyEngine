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
#include <gltf/gltf_asset.h>
#include <gltf/gltf_loader.h>
#include <gltf/resource_loader.h>
#include <rpe/ibl.h>
#include <rpe/material.h>
#include <rpe/renderable_manager.h>
#include <rpe/object_manager.h>
#include <rpe/settings.h>
#include <rpe/skybox.h>
#include <rpe/light_manager.h>
#include <utility/filesystem.h>

#include <parg.h>
#include <stdlib.h>

void print_usage()
{
    printf("Usage:\n");
    printf("gltf_viewer [OPTIONS] <GLTF_MODEL_PATH> \n");
    printf("--eqi-rect \t Eqirect HDR image for IBL in either png or jpg format.\n");
    printf("--cubemap \t HDR cube-map for IBL in ktx format.\n");
    printf("--win-width\t Window width in pixels\n");
    printf("--win-height\t Window height in pixels\n");
    printf("--disable-skybox\t Disables the rendering of the skybox when a IBL cubemap is specified.\n");
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        print_usage();
        exit(0);
    }

    const char* gltf_model_path = NULL;
    uint32_t win_width = 1920;
    uint32_t win_height = 1080;
    const char* ibl_eqirect_image = NULL;
    const char* ibl_cubemap_image = NULL;
    bool draw_skybox = true;

    struct parg_state ps;
    parg_init(&ps);

    int opt;
    while ((opt = parg_getopt(&ps, argc, argv, "he:c:w:h:s")) != -1)
    {
        switch (opt)
        {
            case 1:
                gltf_model_path = ps.optarg;
                break;
            case 'h':
                print_usage();
                exit(0);
            case 'e':
                ibl_eqirect_image = ps.optarg;
                break;
            case 'c':
                ibl_cubemap_image = ps.optarg;
                break;
            case 'w':
                win_width = strtol(ps.optarg, NULL, 0);
                break;
            case 't':
                win_height = strtol(ps.optarg, NULL, 0);
                break;
            case 's':
                draw_skybox = false;
                break;
            default:
                log_error("error: unhandled option -%c\n", opt);
                return EXIT_FAILURE;
        }
    }

    if (!gltf_model_path)
    {
        printf("Gltf model path not specified.\n");
        print_usage();
        exit(1);
    }

    rpe_settings_t settings = {
        .gbuffer_dims = 2048,
        .draw_shadows = false};

    rpe_app_t app;
    int error = rpe_app_init("GLTF Viewer", win_width, win_height, &app, &settings, false);
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
    rpe_renderer_t* renderer = rpe_engine_create_renderer(app.engine);

    // Add a directional light for shadows.
    rpe_object_t dir_obj = rpe_obj_manager_create_obj(om);
    rpe_light_create_info_t ci = {.position = 0.7f, -1.0f, -0.8f};
    rpe_light_manager_create_light(lm, &ci, dir_obj, RPE_LIGHTING_TYPE_DIRECTIONAL);

    // IBL env maps.
    if (ibl_eqirect_image || ibl_cubemap_image)
    {
        struct PreFilterOptions pf_options = {
            .specular_level_count = 8, .brdf_sample_count = 1024, .specular_sample_count = 32};

        ibl_t* ibl = rpe_ibl_init(app.engine, app.scene, pf_options);
        if (!ibl)
        {
            exit(1);
        }

        bool res = ibl_eqirect_image
            ? ibl_load_eqirect_hdr_image(ibl, app.engine, ibl_eqirect_image)
            : ibl_load_cubemap_ktx(ibl, app.engine, ibl_cubemap_image);
        if (!res)
        {
            exit(1);
        }
        rpe_ibl_create_env_maps(ibl, app.engine);
        rpe_scene_set_ibl(app.scene, ibl);

        if (draw_skybox)
        {
            rpe_skybox_t* skybox = rpe_engine_create_skybox(app.engine);
            rpe_skybox_set_cubemap_from_ibl(skybox, ibl, app.engine);
            rpe_scene_set_current_skyox(app.scene, skybox);
        }
    }

    // GLTF model parsing.
    FILE* fp = fopen(gltf_model_path, "rb");
    if (!fp)
    {
        log_error("Unable to open gltf model at path: %s", gltf_model_path);
        exit(1);
    }
    size_t file_sz = fs_get_file_size(fp);
    uint8_t* buffer = malloc(file_sz);
    fread(buffer, sizeof(uint8_t), file_sz, fp);

    gltf_asset_t* asset = gltf_model_parse_data(buffer, file_sz, app.engine, gltf_model_path, &app.scratch_arena);
    if (!asset)
    {
        exit(1);
    }

    gltf_resource_loader_load_textures(asset, app.engine, &app.scratch_arena);
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

    rpe_camera_view_set_camera_type(&app.window.cam_view, RPE_CAMERA_FIRST_PERSON);
    rpe_camera_view_set_position(&app.window.cam_view, math_vec3f_init(0.0f, 0.0f, -1.0f));

    rpe_app_run(&app, renderer, NULL, NULL, NULL, NULL, NULL);

    rpe_app_shutdown(&app);

    exit(0);
}
