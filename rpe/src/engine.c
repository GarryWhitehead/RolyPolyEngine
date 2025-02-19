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

#include "engine.h"

#include "camera.h"
#include "managers/light_manager.h"
#include "managers/object_manager.h"
#include "managers/renderable_manager.h"
#include "managers/transform_manager.h"
#include "renderer.h"
#include "scene.h"
#include "vertex_buffer.h"

#include <assert.h>
#include <log.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/error_codes.h>

rpe_engine_t* rpe_engine_create(vkapi_driver_t* driver)
{
    assert(driver);

    rpe_engine_t* instance = calloc(1, sizeof(struct Engine));
    assert(instance);

    instance->driver = driver;
    int err = arena_new(RPE_ENGINE_SCRATCH_ARENA_SIZE, &instance->scratch_arena);
    assert(err == ARENA_SUCCESS);
    err = arena_new(RPE_ENGINE_PERM_ARENA_SIZE, &instance->perm_arena);
    assert(err == ARENA_SUCCESS);
    err = arena_new(RPE_ENGINE_FRAME_ARENA_SIZE, &instance->frame_arena);
    assert(err == ARENA_SUCCESS);

    MAKE_DYN_ARRAY(vkapi_swapchain_t, &instance->perm_arena, 10, &instance->swapchains);
    MAKE_DYN_ARRAY(rpe_renderer_t*, &instance->perm_arena, 10, &instance->renderers);
    MAKE_DYN_ARRAY(rpe_renderable_t, &instance->perm_arena, 100, &instance->renderables);
    MAKE_DYN_ARRAY(rpe_scene_t*, &instance->perm_arena, 10, &instance->scenes);
    MAKE_DYN_ARRAY(rpe_camera_t*, &instance->perm_arena, 10, &instance->cameras);

    // Load the material shaders. Held by the engine as the most logical place.
    instance->mat_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX] = program_cache_from_spirv(
        driver->prog_manager,
        driver->context,
        "material.vert.spv",
        RPE_BACKEND_SHADER_STAGE_VERTEX,
        &instance->perm_arena);
    instance->mat_shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT] = program_cache_from_spirv(
        driver->prog_manager,
        driver->context,
        "material.frag.spv",
        RPE_BACKEND_SHADER_STAGE_FRAGMENT,
        &instance->perm_arena);

    if ((!vkapi_is_valid_shader_handle(instance->mat_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX]) ||
         (!vkapi_is_valid_shader_handle(instance->mat_shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT]))))
    {
        return NULL;
    }

    instance->camera_ubo =
        vkapi_res_cache_create_ubo(driver->res_cache, driver, sizeof(rpe_camera_ubo_t));

    instance->obj_manager = rpe_obj_manager_init(&instance->perm_arena);
    instance->transform_manager = rpe_transform_manager_init(instance, &instance->perm_arena);
    instance->rend_manager = rpe_rend_manager_init(instance, &instance->perm_arena);
    instance->light_manager = rpe_light_manager_init(instance, &instance->perm_arena);
    instance->vbuffer = rpe_vertex_buffer_init(driver, &instance->perm_arena);

    // Create dummy textures - only needed for bound samplers to prevent validation warnings.
    uint32_t zero_buffer[6] = {0};
    sampler_params_t sampler = {
        .addr_u = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE,
        .addr_v = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE};
    instance->tex_dummy_cubemap = vkapi_res_cache_create_tex2d(
        driver->res_cache,
        driver->context,
        driver->sampler_cache,
        VK_FORMAT_R8G8B8A8_UNORM,
        1,
        1,
        1,
        6,
        1,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        &sampler);
    instance->tex_dummy = vkapi_res_cache_create_tex2d(
        driver->res_cache,
        driver->context,
        driver->sampler_cache,
        VK_FORMAT_R8G8B8A8_UNORM,
        1,
        1,
        1,
        1,
        1,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        &sampler);

    return instance;
}

void rpe_engine_shutdown(rpe_engine_t* engine)
{
    for (uint32_t i = 0; i < engine->swapchains.size; ++i)
    {
        vkapi_swapchain_t* sc = DYN_ARRAY_GET_PTR(vkapi_swapchain_t, &engine->swapchains, i);
        vkapi_swapchain_destroy(engine->driver, sc);
    }

    arena_release(&engine->perm_arena);
    arena_release(&engine->scratch_arena);
    arena_release(&engine->frame_arena);
    free(engine);
    engine = NULL;
}

swapchain_handle_t* rpe_engine_create_swapchain(
    rpe_engine_t* engine, VkSurfaceKHR surface, uint32_t width, uint32_t height)
{
    vkapi_swapchain_t sc = vkapi_swapchain_init();
    int err =
        vkapi_swapchain_create(engine->driver, &sc, surface, width, height, &engine->scratch_arena);

    if (err != VKAPI_SUCCESS)
    {
        log_error("Error creating swapchain.");
        return NULL;
    }

    swapchain_handle_t* handle = calloc(1, sizeof(struct SwapchainHandle));
    handle->idx = engine->swapchains.size;
    DYN_ARRAY_APPEND(&engine->swapchains, &sc);
    return handle;
}

rpe_renderer_t* rpe_engine_create_renderer(rpe_engine_t* engine)
{
    assert(engine);
    rpe_renderer_t* rend = rpe_renderer_init(engine, &engine->perm_arena);
    DYN_ARRAY_APPEND(&engine->renderers, &rend);
    return rend;
}

rpe_scene_t* rpe_engine_create_scene(rpe_engine_t* engine)
{
    assert(engine);
    rpe_scene_t* scene = rpe_scene_init(engine, &engine->perm_arena);
    DYN_ARRAY_APPEND(&engine->scenes, &scene);
    return scene;
}

rpe_camera_t* rpe_engine_create_camera(
    rpe_engine_t* engine, float fovy, float aspect, float n, float f, enum ProjectionType type)
{
    assert(engine);
    rpe_camera_t* cam = rpe_camera_init(engine, fovy, aspect, n, f, type);
    DYN_ARRAY_APPEND(&engine->cameras, &cam);
    return cam;
}

rpe_renderable_t*
rpe_engine_create_renderable(rpe_engine_t* engine, rpe_material_t* mat, rpe_mesh_t* mesh)
{
    assert(engine);
    rpe_renderable_t rend = rpe_renderable_init();
    rend.mesh_data = mesh;
    rend.material = mat;
    rpe_material_update_vertex_constants(mat, mesh);
    return DYN_ARRAY_APPEND(&engine->renderables, &rend);
}

bool rpe_engine_destroy_scene(rpe_engine_t* engine, rpe_scene_t* scene)
{
    for (size_t i = 0; i < engine->scenes.size; ++i)
    {
        rpe_scene_t* ptr = DYN_ARRAY_GET(rpe_scene_t*, &engine->scenes, i);
        if (ptr == scene)
        {
            DYN_ARRAY_REMOVE(&engine->scenes, i);
            // TODO: add some way of returning allocated scene ptr back to arena space.
            engine->curr_scene = engine->curr_scene == ptr ? NULL : engine->curr_scene;
            return true;
        }
    }
    return false;
}

bool rpe_engine_destroy_camera(rpe_engine_t* engine, rpe_camera_t* camera)
{
    for (size_t i = 0; i < engine->cameras.size; ++i)
    {
        rpe_camera_t* ptr = DYN_ARRAY_GET(rpe_camera_t*, &engine->cameras, i);
        if (ptr == camera)
        {
            DYN_ARRAY_REMOVE(&engine->cameras, i);
            // TODO: add some way of returning allocated scene ptr back to arena space.
            if (engine->curr_scene && engine->curr_scene->curr_camera == ptr)
            {
                engine->curr_scene->curr_camera = NULL;
            }
            return true;
        }
    }
    return false;
}

bool rpe_engine_destroy_renderer(rpe_engine_t* engine, rpe_renderer_t* renderer)
{
    for (size_t i = 0; i < engine->renderers.size; ++i)
    {
        rpe_renderer_t* ptr = DYN_ARRAY_GET(rpe_renderer_t*, &engine->renderers, i);
        if (ptr == renderer)
        {
            DYN_ARRAY_REMOVE(&engine->renderers, i);
            // TODO: add some way of returning allocated scene ptr back to arena space.
            return true;
        }
    }
    return false;
}

/** Public functions **/

void rpe_engine_set_current_scene(rpe_engine_t* engine, rpe_scene_t* scene)
{
    assert(engine);
    engine->curr_scene = scene;
}

void rpe_engine_set_current_swapchain(rpe_engine_t* engine, swapchain_handle_t* handle)
{
    assert(engine);
    vkapi_swapchain_t* sc = DYN_ARRAY_GET_PTR(vkapi_swapchain_t, &engine->swapchains, handle->idx);
    engine->curr_swapchain = sc;
}

rpe_obj_manager_t* rpe_engine_get_obj_manager(rpe_engine_t* engine)
{
    assert(engine);
    return engine->obj_manager;
}

rpe_rend_manager_t* rpe_engine_get_rend_manager(rpe_engine_t* engine)
{
    assert(engine);
    return engine->rend_manager;
}

rpe_transform_manager_t* rpe_engine_get_transform_manager(rpe_engine_t* engine)
{
    assert(engine);
    return engine->transform_manager;
}
