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

#include "ibl.h"

#include "camera.h"
#include "engine.h"
#include "material.h"
#include "renderer.h"
#include "rpe/engine.h"
#include "rpe/renderer.h"
#include "rpe/scene.h"
#include "scene.h"

#include <backend/enums.h>
#include <log.h>
#include <utility/arena.h>
#include <vulkan-api/driver.h>

void generate_face_views(math_mat4f* face_views)
{
    {
        // X_POSITIVE
        math_vec3f target = {1.0f, 0.0f, 0.0f};
        math_vec3f eye = {0.0f, 0.0f, 0.0f};
        math_vec3f up = {0.0f, -1.0f, 0.0f};
        face_views[0] = math_mat4f_lookat(target, eye, up);
    }
    {
        // X_NEGATIVE
        math_vec3f target = {-1.0f, 0.0f, 0.0f};
        math_vec3f eye = {0.0f, 0.0f, 0.0f};
        math_vec3f up = {0.0f, -1.0f, 0.0f};
        face_views[1] = math_mat4f_lookat(target, eye, up);
    }
    {
        // Y-POSITIVE
        math_vec3f target = {0.0f, -1.0f, 0.0f};
        math_vec3f eye = {0.0f, 0.0f, 0.0f};
        math_vec3f up = {0.0f, 0.0f, -1.0f};
        face_views[2] = math_mat4f_lookat(target, eye, up);
    }
    {
        // Y_NEGATIVE
        math_vec3f target = {0.0f, 1.0f, 0.0f};
        math_vec3f eye = {0.0f, 0.0f, 0.0f};
        math_vec3f up = {0.0f, 0.0f, 1.0f};
        face_views[3] = math_mat4f_lookat(target, eye, up);
    }
    {
        // Z_POSITIVE
        math_vec3f target = {0.0f, 0.0f, 1.0f};
        math_vec3f eye = {0.0f, 0.0f, 0.0f};
        math_vec3f up = {0.0f, -1.0f, 0.0f};
        face_views[4] = math_mat4f_lookat(target, eye, up);
    }
    {
        // Z_NEGATIVE
        math_vec3f target = {0.0f, 0.0f, -1.0f};
        math_vec3f eye = {0.0f, 0.0f, 0.0f};
        math_vec3f up = {0.0f, -1.0f, 0.0f};
        face_views[5] = math_mat4f_lookat(target, eye, up);
    }
}

texture_handle_t ibl_eqirect_to_cubemap(
    ibl_t* ibl,
    rpe_engine_t* engine,
    shader_prog_bundle_t* bundle,
    void* hdr_image,
    uint32_t hdr_width,
    uint32_t hdr_height)
{
    vkapi_driver_t* driver = engine->driver;
    uint32_t sz = hdr_width * hdr_height * 4 * sizeof(float);
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

    // Upload the eqirect image to the GPU.
    sampler_params_t sampler = {
        .addr_u = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE,
        .addr_v = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE};
    texture_handle_t eqi_tex = vkapi_res_cache_create_tex2d(
        driver->res_cache,
        driver->context,
        driver->sampler_cache,
        format,
        hdr_width,
        hdr_height,
        1,
        1,
        1,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        &sampler);
    vkapi_driver_map_gpu_texture(driver, eqi_tex, hdr_image, sz, NULL, false);

    shader_bundle_add_image_sampler(bundle, driver, eqi_tex, 0);

    // Create the cube texture to render into.
    texture_handle_t target_handle = vkapi_res_cache_create_tex2d(
        driver->res_cache,
        driver->context,
        driver->sampler_cache,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        RPE_IBL_EQIRECT_CUBEMAP_DIMENSIONS,
        RPE_IBL_EQIRECT_CUBEMAP_DIMENSIONS,
        0xffff,
        6,
        1,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        &sampler);

    // set the empty cube map as the render target for our draws
    rpe_render_target_t rt = rpe_render_target_init();
    rt.attachments[0].handle = target_handle;
    rt.load_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    rt.store_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE;

    shader_bundle_set_viewport(
        bundle, RPE_IBL_EQIRECT_CUBEMAP_DIMENSIONS, RPE_IBL_EQIRECT_CUBEMAP_DIMENSIONS, 0.0f, 1.0f);

    rpe_renderer_render_single_indexed(
        ibl->renderer, &rt, bundle, ibl->cubemap_vertices, ibl->cubemap_indices, 36, NULL, 0, true);

    uint32_t mip_levels = rpe_material_max_mipmaps(
        RPE_IBL_EQIRECT_CUBEMAP_DIMENSIONS, RPE_IBL_EQIRECT_CUBEMAP_DIMENSIONS);
    vkapi_driver_generate_mipmaps(driver, target_handle, mip_levels);
    vkapi_driver_flush_gfx_cmds(driver);

    vkapi_driver_destroy_rt(driver, &rt.handle);
    vkapi_res_cache_delete_tex2d(driver->res_cache, eqi_tex);
    return target_handle;
}

texture_handle_t ibl_create_brdf_lut(ibl_t* ibl, rpe_engine_t* engine)
{
    vkapi_driver_t* driver = engine->driver;

    struct BrdfUBO ubo = {.sample_count = ibl->options.brdf_sample_count};
    vkapi_driver_map_gpu_buffer(engine->driver, ibl->brdf_ubo, sizeof(struct BrdfUBO), 0, &ubo);

    // create the cube texture to render into
    sampler_params_t sampler = {
        .addr_u = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE,
        .addr_v = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE};

    texture_handle_t target_handle = vkapi_res_cache_create_tex2d(
        driver->res_cache,
        driver->context,
        driver->sampler_cache,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        RPE_IBL_EQIRECT_CUBEMAP_DIMENSIONS,
        RPE_IBL_EQIRECT_CUBEMAP_DIMENSIONS,
        1,
        1,
        1,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &sampler);

    // Set the empty target texture as the render target for the draw.
    rpe_render_target_t rt = rpe_render_target_init();
    rt.attachments[0].handle = target_handle;
    rt.load_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    rt.store_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE;

    rpe_renderer_render_single_quad(ibl->renderer, &rt, ibl->brdf.bundle, false);

    vkapi_driver_destroy_rt(driver, &rt.handle);
    return target_handle;
}

texture_handle_t
ibl_create_irradiance_env_map(ibl_t* ibl, rpe_engine_t* engine, texture_handle_t cube_map)
{
    vkapi_driver_t* driver = engine->driver;

    shader_bundle_add_image_sampler(ibl->irradiance_envmap.bundle, driver, cube_map, 0);

    // create the cube texture to render into
    sampler_params_t sampler = {
        .addr_u = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE,
        .addr_v = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE};

    texture_handle_t target_handle = vkapi_res_cache_create_tex2d(
        driver->res_cache,
        driver->context,
        driver->sampler_cache,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        RPE_IBL_IRRADIANCE_ENV_MAP_DIMENSIONS,
        RPE_IBL_IRRADIANCE_ENV_MAP_DIMENSIONS,
        1,
        6,
        1,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &sampler);

    // Set the empty target texture as the render target for the draw.
    rpe_render_target_t rt = rpe_render_target_init();
    rt.attachments[0].handle = target_handle;
    rt.load_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    rt.store_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE;

    shader_bundle_set_viewport(
        ibl->irradiance_envmap.bundle,
        RPE_IBL_IRRADIANCE_ENV_MAP_DIMENSIONS,
        RPE_IBL_IRRADIANCE_ENV_MAP_DIMENSIONS,
        0.0f,
        1.0f);

    rpe_renderer_render_single_indexed(
        ibl->renderer,
        &rt,
        ibl->irradiance_envmap.bundle,
        ibl->cubemap_vertices,
        ibl->cubemap_indices,
        36,
        NULL,
        0,
        true);
    vkapi_driver_destroy_rt(driver, &rt.handle);

    return target_handle;
}

texture_handle_t
ibl_create_specular_env_map(ibl_t* ibl, rpe_engine_t* engine, texture_handle_t cube_map)
{
    vkapi_driver_t* driver = engine->driver;

    struct SpecularFragUBO ubo = {.sample_count = ibl->options.specular_sample_count};
    vkapi_driver_map_gpu_buffer(
        engine->driver, ibl->specular_frag_ubo, sizeof(struct SpecularFragUBO), 0, &ubo);

    shader_bundle_add_image_sampler(ibl->specular.bundle, driver, cube_map, 0);

    // create the cube texture to render into
    sampler_params_t sampler = {
        .addr_u = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE,
        .addr_v = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE};

    texture_handle_t target_handle = vkapi_res_cache_create_tex2d(
        driver->res_cache,
        driver->context,
        driver->sampler_cache,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        RPE_IBL_SPECULAR_ENV_MAP_DIMENSIONS,
        RPE_IBL_SPECULAR_ENV_MAP_DIMENSIONS,
        ibl->options.specular_level_count,
        6,
        1,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        &sampler);

    // Set the empty target texture as the render target for the draw.
    rpe_render_target_t rt = rpe_render_target_init();
    rt.attachments[0].handle = target_handle;
    rt.load_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    rt.store_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE;

    vkapi_driver_transition_image(
        driver,
        target_handle,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        ibl->options.specular_level_count);

    // Render each cube-map mip level (faces are drawn in one call with multiview)
    uint32_t dim = RPE_IBL_SPECULAR_ENV_MAP_DIMENSIONS;
    uint32_t max_mip_levels = ibl->options.specular_level_count;

    for (uint32_t level = 0; level < max_mip_levels; ++level)
    {
        rt.attachments[0].mipLevel = level;
        shader_bundle_set_viewport(ibl->specular.bundle, dim, dim, 0.0f, 1.0f);

        float roughness = (float)level / (float)(max_mip_levels - 1);
        struct PushBlockEntry entry = {
            .data = &roughness, .stage = RPE_BACKEND_SHADER_STAGE_FRAGMENT};

        rpe_renderer_render_single_indexed(
            ibl->renderer,
            &rt,
            ibl->specular.bundle,
            ibl->cubemap_vertices,
            ibl->cubemap_indices,
            36,
            &entry,
            1,
            true);
        vkapi_driver_destroy_rt(driver, &rt.handle);

        dim >>= 1;
    }
    // Prepare image (and all mip levels) for shader reads.
    vkapi_driver_transition_image(
        driver,
        target_handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        ibl->options.specular_level_count);

    return target_handle;
}

bool rpe_ibl_upload_cubemap(
    ibl_t* ibl,
    rpe_engine_t* engine,
    const float* image_data,
    uint32_t image_sz,
    uint32_t width,
    uint32_t height,
    uint32_t mip_levels,
    size_t* mip_offsets)
{
    vkapi_driver_t* driver = engine->driver;

    if (!image_data || !image_sz)
    {
        log_error("Invalid image parameters.");
        return false;
    }

    sampler_params_t sampler = {
        .addr_u = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE,
        .addr_v = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE};

    bool gen_mipmaps = mip_levels <= 1 ? true : false;
    uint32_t levels = gen_mipmaps ? rpe_material_max_mipmaps(width, height) : mip_levels;

    ibl->tex_cube_map = vkapi_res_cache_create_tex2d(
        driver->res_cache,
        driver->context,
        driver->sampler_cache,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        width,
        height,
        levels,
        6,
        1,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        &sampler);
    vkapi_driver_map_gpu_texture(
        driver, ibl->tex_cube_map, (void*)image_data, image_sz, mip_offsets, gen_mipmaps);

    return true;
}

bool rpe_ibl_eqirect_to_cubemap(
    ibl_t* ibl, rpe_engine_t* engine, float* image_data, uint32_t width, uint32_t height)
{
    if (!vkapi_tex_handle_is_valid(ibl->tex_cube_map))
    {
        log_error("Cubemap is not valid. The cubemap must have been created from a eqirect image "
                  "or uploaded to the device before calling this function");
        return false;
    }

    vkapi_driver_t* driver = engine->driver;

    shader_prog_bundle_t* bundle =
        program_cache_create_program_bundle(driver->prog_manager, &engine->perm_arena);
    shader_handle_t handles[2];

    handles[0] = program_cache_from_spirv(
        driver->prog_manager,
        driver->context,
        "eqirect_to_cubemap.vert.spv",
        RPE_BACKEND_SHADER_STAGE_VERTEX,
        &engine->perm_arena);
    handles[1] = program_cache_from_spirv(
        driver->prog_manager,
        driver->context,
        "eqirect_to_cubemap.frag.spv",
        RPE_BACKEND_SHADER_STAGE_FRAGMENT,
        &engine->perm_arena);

    if ((!vkapi_is_valid_shader_handle(handles[0]) || (!vkapi_is_valid_shader_handle(handles[1]))))
    {
        return false;
    }

    shader_bundle_update_ubo_desc(bundle, 0, engine->camera_ubo);
    shader_bundle_update_ubo_desc(bundle, 1, ibl->faceview_ubo);

    shader_bundle_add_vertex_input_binding(
        bundle,
        handles[0],
        driver,
        0, // First location
        0, // End location.
        0, // Binding id.
        VK_VERTEX_INPUT_RATE_VERTEX);

    ibl->tex_cube_map = ibl_eqirect_to_cubemap(ibl, engine, bundle, image_data, width, height);

    return true;
}

bool rpe_ibl_create_env_maps(ibl_t* ibl, rpe_engine_t* engine)
{
    assert(ibl);

    vkapi_driver_t* driver = engine->driver;

    const char* vert_shader_filenames[3] = {
        "fullscreen_quad.vert.spv", "specular_prefilter.vert.spv", "irradiance_envmap.vert.spv"};
    const char* frag_shader_filenames[3] = {
        "brdf.frag.spv", "specular_prefilter.frag.spv", "irradiance_envmap.frag.spv"};
    struct IblBundle* bundles[3] = {&ibl->brdf, &ibl->specular, &ibl->irradiance_envmap};

    for (int i = 0; i < 3; ++i)
    {
        bundles[i]->handles[RPE_BACKEND_SHADER_STAGE_VERTEX] = program_cache_from_spirv(
            driver->prog_manager,
            driver->context,
            vert_shader_filenames[i],
            RPE_BACKEND_SHADER_STAGE_VERTEX,
            &engine->perm_arena);
        bundles[i]->handles[RPE_BACKEND_SHADER_STAGE_FRAGMENT] = program_cache_from_spirv(
            driver->prog_manager,
            driver->context,
            frag_shader_filenames[i],
            RPE_BACKEND_SHADER_STAGE_FRAGMENT,
            &engine->perm_arena);

        if ((!vkapi_is_valid_shader_handle(bundles[i]->handles[RPE_BACKEND_SHADER_STAGE_VERTEX]) ||
             (!vkapi_is_valid_shader_handle(
                 bundles[i]->handles[RPE_BACKEND_SHADER_STAGE_FRAGMENT]))))
        {
            return false;
        }

        bundles[i]->bundle =
            program_cache_create_program_bundle(driver->prog_manager, &engine->perm_arena);

        shader_bundle_update_descs_from_reflection(
            bundles[i]->bundle,
            driver,
            bundles[i]->handles[RPE_BACKEND_SHADER_STAGE_VERTEX],
            &engine->perm_arena);
        shader_bundle_update_descs_from_reflection(
            bundles[i]->bundle,
            driver,
            bundles[i]->handles[RPE_BACKEND_SHADER_STAGE_FRAGMENT],
            &engine->perm_arena);
    }

    // BRDF UBOs.
    ibl->brdf_ubo = vkapi_res_cache_create_ubo(driver->res_cache, driver, sizeof(struct BrdfUBO));
    shader_bundle_update_ubo_desc(ibl->brdf.bundle, 0, ibl->brdf_ubo);

    // Irradiance
    shader_bundle_update_ubo_desc(ibl->irradiance_envmap.bundle, 0, engine->camera_ubo);
    shader_bundle_update_ubo_desc(ibl->irradiance_envmap.bundle, 1, ibl->faceview_ubo);

    shader_bundle_add_vertex_input_binding(
        ibl->irradiance_envmap.bundle,
        ibl->irradiance_envmap.handles[RPE_BACKEND_SHADER_STAGE_VERTEX],
        driver,
        0, // First location
        0, // End location.
        0, // Binding id.
        VK_VERTEX_INPUT_RATE_VERTEX);

    // Specular prefilter UBOS.
    ibl->specular_frag_ubo =
        vkapi_res_cache_create_ubo(driver->res_cache, driver, sizeof(struct SpecularFragUBO));
    shader_bundle_create_push_block(
        ibl->specular.bundle, sizeof(float), RPE_BACKEND_SHADER_STAGE_FRAGMENT);
    shader_bundle_update_ubo_desc(ibl->specular.bundle, 0, engine->camera_ubo);
    shader_bundle_update_ubo_desc(ibl->specular.bundle, 1, ibl->faceview_ubo);
    shader_bundle_update_ubo_desc(ibl->specular.bundle, 2, ibl->specular_frag_ubo);

    shader_bundle_add_vertex_input_binding(
        ibl->specular.bundle,
        ibl->specular.handles[RPE_BACKEND_SHADER_STAGE_VERTEX],
        driver,
        0, // First location
        0, // End location.
        0, // Binding id.
        VK_VERTEX_INPUT_RATE_VERTEX);

    // Create the env maps and BRDF look-up table.
    ibl->tex_irradiance_map = ibl_create_irradiance_env_map(ibl, engine, ibl->tex_cube_map);
    ibl->tex_specular_map = ibl_create_specular_env_map(ibl, engine, ibl->tex_cube_map);
    ibl->tex_brdf_lut = ibl_create_brdf_lut(ibl, engine);

    vkapi_driver_flush_gfx_cmds(engine->driver);
    rpe_engine_destroy_renderer(engine, ibl->renderer);
    rpe_engine_destroy_camera(engine, ibl->camera);

    return true;
}

ibl_t* rpe_ibl_init(rpe_engine_t* engine, struct PreFilterOptions options)
{
    assert(options.brdf_sample_count > 0);
    assert(options.specular_level_count > 0);
    assert(options.specular_sample_count > 0);

    ibl_t* ibl = ARENA_MAKE_ZERO_STRUCT(&engine->perm_arena, ibl_t);
    ibl->tex_cube_map.id = UINT32_MAX;

    vkapi_driver_t* driver = engine->driver;

    ibl->camera = rpe_engine_create_camera(
        engine, 90.0f, 1.0f, 0.1f, 512.0f, RPE_PROJECTION_TYPE_PERSPECTIVE);
    rpe_camera_ubo_t cam_ubo = rpe_camera_update_ubo(ibl->camera, NULL);
    vkapi_driver_map_gpu_buffer(
        engine->driver, engine->camera_ubo, sizeof(rpe_camera_ubo_t), 0, &cam_ubo);

    ibl->renderer = rpe_engine_create_renderer(engine);
    ibl->options = options;

    // Setup attributes which are shared amongst ibl prefilter objects.
    // clang-format off
    math_vec3f cube_vertices[8] = {
        {-1.0f, -1.0f, 1.0f},  {1.0f, -1.0f, 1.0f},
        {1.0f,  1.0f, 1.0f},   {-1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f},
        {1.0f, 1.0f, -1.0f},   {-1.0f, 1.0f,  -1.0f}};

    // cube indices
    uint32_t cube_indices[36] = {
        0, 1, 2, 2, 3, 0,       // front
        1, 5, 6, 6, 2, 1,       // right side
        7, 6, 5, 5, 4, 7,       // left side
        4, 0, 3, 3, 7, 4,       // bottom
        4, 5, 1, 1, 0, 4,       // back
        3, 2, 6, 6, 7, 3        // top
    };
    // clang-format on

    ibl->cubemap_vertices =
        vkapi_res_cache_create_vertex_buffer(driver->res_cache, driver, sizeof(math_vec3f) * 8);
    ibl->cubemap_indices =
        vkapi_res_cache_create_index_buffer(driver->res_cache, driver, sizeof(uint32_t) * 36);

    vkapi_driver_map_gpu_vertex(
        driver,
        ibl->cubemap_vertices,
        cube_vertices,
        8 * sizeof(math_vec3f),
        ibl->cubemap_indices,
        cube_indices,
        36 * sizeof(uint32_t));

    // The face views UBO - generate and upload to the device.
    struct FaceviewUBO ubo;
    generate_face_views(ubo.face_views);
    ibl->faceview_ubo =
        vkapi_res_cache_create_ubo(driver->res_cache, driver, sizeof(struct FaceviewUBO));
    vkapi_driver_map_gpu_buffer(
        engine->driver, ibl->faceview_ubo, sizeof(struct FaceviewUBO), 0, &ubo);

    return ibl;
}
