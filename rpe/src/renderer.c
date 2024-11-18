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

#include "renderer.h"

#include "colour_pass.h"
#include "engine.h"
#include "material.h"
#include "render_graph/render_graph.h"
#include "render_graph/render_graph_handle.h"
#include "scene.h"

#include <utility/arena.h>
#include <vulkan-api/renderpass.h>
#include <vulkan-api/resource_cache.h>

rpe_renderer_t* rpe_renderer_init(rpe_engine_t* engine, arena_t* arena)
{
    assert(engine);
    rpe_renderer_t* i = ARENA_MAKE_ZERO_STRUCT(arena, rpe_renderer_t);
    vkapi_swapchain_t* sc = engine->curr_swapchain;
    assert(sc);

    // Create the back-buffer depth texture.
    VkFormat depth_format = vkapi_driver_get_supported_depth_format(engine->driver);

    i->depth_handle = vkapi_res_cache_create_tex2d(
        engine->driver->res_cache,
        engine->driver->context,
        depth_format,
        sc->extent.width,
        sc->extent.height,
        1,
        1,
        1,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // assume triple buffered backbuffer
    for (uint8_t idx = 0; idx < 3; ++idx)
    {
        vkapi_attach_info_t colour[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT];
        colour[0].layer = 0;
        colour[0].level = 0;
        colour[0].handle = sc->contexts[idx].texture;

        vkapi_attach_info_t depth = {.level = 0, .layer = 0, .handle = i->depth_handle};
        math_vec4f col = {0};
        vkapi_attach_info_t stencil = {0};
        i->rt_handles[idx] =
            vkapi_driver_create_rt(engine->driver, false, col, colour, depth, stencil);
    }
    return i;
}

void rpe_renderer_begin_frame(rpe_renderer_t* r)
{
    assert(r);
    vkapi_swapchain_t* sc = r->engine->curr_swapchain;
    bool success = vkapi_driver_begin_frame(r->engine->driver, sc);
    // TODO: need to handle out of date swapchains here
    assert(success);
}

void rpe_renderer_end_frame(rpe_renderer_t* r)
{
    assert(r);
    vkapi_swapchain_t* sc = r->engine->curr_swapchain;
    vkapi_driver_end_frame(r->engine->driver, sc);
}

void rpe_renderer_render_single_scene(
    rpe_renderer_t* rdr, vkapi_driver_t* driver, rpe_scene_t* scene, rpe_render_target_t* rt)
{
    vkapi_cmdbuffer_t* cmds = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);

    vkapi_render_pass_data_t data;
    data.width = rt->width;
    data.height = rt->height;

    memcpy(
        data.load_clear_flags,
        rt->load_flags,
        sizeof(enum LoadClearFlags) * VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT);
    memcpy(
        data.store_clear_flags,
        rt->store_flags,
        sizeof(enum StoreClearFlags) * VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT);

    rpe_scene_update(scene, rdr->engine);

    vkapi_driver_begin_rpass(driver, cmds->instance, &data, &rt->handle);
    rpe_render_queue_submit(&scene->render_queue, driver);
    vkapi_driver_end_rpass(cmds->instance);
}

void rpe_renderer_render(
    rpe_renderer_t* rdr, vkapi_driver_t* driver, rpe_engine_t* engine, rpe_scene_t* scene, float dt, bool clearSwap)
{
    rg_clear(rdr->rg);

    // Update the renderable objects and lights.
    rpe_scene_update(scene, rdr->engine);

    vkapi_swapchain_t* sc = rdr->engine->curr_swapchain;

    // Resource input which will be moved to the back-buffer RT.
    rg_handle_t input_handle;

    // Import the back-buffer render target into the render graph.
    rg_import_rt_desc_t desc;
    desc.width = sc->extent.width;
    desc.height = sc->extent.height;
    desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Store/clear flags for final colour attachment.
    desc.store_clear_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE;
    desc.load_clear_flags[0] = clearSwap ? RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR
                                         : RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_LOAD;
    desc.final_layouts[0] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    desc.init_layouts[0] = clearSwap ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // TODO: should be definable via the client api
    desc.clear_col.r = 0.0f;
    desc.clear_col.g = 0.0f;
    desc.clear_col.b = 0.0f;
    desc.clear_col.a = 1.0f;

    rg_handle_t bb_handle =
        rg_import_render_target(rdr->rg, "BackBuffer", desc, rdr->rt_handles[driver->image_index]);

    VkFormat depth_format = vkapi_driver_get_supported_depth_format(rdr->engine->driver);

    // Fill the gbuffers - this can't be the final render target unless gbuffers are disabled due
    // to the gBuffers requiring resolviong down to a single render target in the lighting pass.
    struct ColourPassInfo cpass_info = rpe_colour_pass_render(rdr->rg);

    rg_move_resource(rdr->rg, input_handle, bb_handle);
    rg_add_present_pass(rdr->rg, bb_handle, "PresentPass");

    rg_compile(rdr->rg);
    rg_execute(rdr->rg, cpass_info.pass, rdr->engine->driver, engine);
}

void rpe_render_target_build(
    rpe_render_target_t* t, rpe_engine_t* engine, const char* name, bool multi_view)
{
    assert(t->attachments[0].texture || t->attachments[VKAPI_RENDER_TARGET_DEPTH_INDEX].texture);

    // convert attachment information to vulkan api format
    vkapi_render_target_t vk_rt;
    vk_rt.samples = t->samples;

    uint32_t min_width = UINT32_MAX;
    uint32_t min_height = UINT32_MAX;
    uint32_t max_width = 0;
    uint32_t max_height = 0;

    for (int i = 0; i < VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT; ++i)
    {
        if (t->attachments[i].texture)
        {
            rpe_mapped_texture_t* tex = t->attachments[i].texture;
            vk_rt.colours[i].handle = tex->backend_handle;
            vk_rt.colours[i].level = t->attachments[i].mipLevel;
            vk_rt.colours[i].layer = t->attachments[i].layer;

            uint32_t width = tex->width >> t->attachments[i].mipLevel;
            uint32_t height = tex->height >> t->attachments[i].mipLevel;
            min_width = MIN(min_width, width) min_height = MIN(min_height, height) max_width =
                MAX(max_width, width) max_height = MAX(max_height, height)
        }
    }
    t->width = min_width;
    t->height = min_height;

    if (t->attachments[VKAPI_RENDER_TARGET_DEPTH_INDEX].texture)
    {
        rpe_mapped_texture_t* tex = t->attachments[VKAPI_RENDER_TARGET_DEPTH_INDEX].texture;
        vk_rt.depth.handle = tex->backend_handle;
        vk_rt.depth.level = t->attachments[VKAPI_RENDER_TARGET_DEPTH_INDEX].mipLevel;
    }

    // Stencil ignored at the moment.
    vkapi_attach_info_t stencil = {0};
    t->handle = vkapi_driver_create_rt(
        engine->driver, multi_view, t->clear_col, vk_rt.colours, vk_rt.depth, stencil);
}
