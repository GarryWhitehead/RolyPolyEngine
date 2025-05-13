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

#include "shadow_pass.h"

#include "engine.h"
#include "render_graph/render_graph.h"
#include "render_graph/render_pass_node.h"
#include "render_graph/rendergraph_resource.h"
#include "render_queue.h"
#include "scene.h"
#include "shadow_manager.h"
#include "vertex_buffer.h"

#include <backend/enums.h>
#include <vulkan-api/commands.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/sampler_cache.h>

void setup_shadow_pass(render_graph_t* rg, rg_pass_node_t* node, void* data, void* local_data)
{
    struct ShadowLocalData* local_d = (struct ShadowLocalData*)local_data;
    struct ShadowPassData* d = (struct ShadowPassData*)data;
    rg_backboard_t* bb = rg_get_backboard(rg);

    rg_texture_desc_t t_desc = {
        .width = local_d->width,
        .height = local_d->height,
        .mip_levels = 1,
        .depth = 1,
        .layers = local_d->cascade_count,
        .format = local_d->depth_format};

    d->depth = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "ShadowDepth", VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);
    d->depth = rg_add_write(rg, d->depth, node, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    rg_backboard_add(bb, "CascadeShadowDepth", d->depth);

    rg_pass_desc_t desc = rg_pass_desc_init();
    desc.attachments.attach.depth = d->depth;
    desc.multi_view_count = local_d->cascade_count;
    desc.ds_load_clear_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    desc.ds_store_clear_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE;
    d->rt = rg_rpass_node_create_rt((rg_render_pass_node_t*)node, rg, "ShadowPass", desc);

    // Only a single writer declared so add side effect otherwise this pass will be culled.
    rg_node_declare_side_effect((rg_node_t*)node);

    d->prog_bundle = local_d->prog_bundle;
    d->scene = local_d->scene;
}

void execute_shadow_pass(
    vkapi_driver_t* driver, rpe_engine_t* engine, rg_render_graph_resource_t* res, void* data)
{
    struct ShadowPassData* d = (struct ShadowPassData*)data;
    rg_resource_info_t info = rg_res_get_render_pass_info(res, d->rt);

    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkapi_driver_begin_rpass(driver, cmd_buffer->instance, &info.data, &info.handle);

    // Bind the uber vertex/index buffers - only one bind call required as all draw calls offset
    // into this buffer.
    // NOTE: The vertex data is currently uploaded in the colour pass so the shadow pass must be
    // called after. If multi threaded rendering is added, this will need changing.
    vkapi_driver_bind_vertex_buffer(driver, engine->vbuffer->vertex_buffer, 0);
    vkapi_driver_bind_vertex_buffer(driver, engine->curr_scene->shadow_model_draw_data_handle, 1);
    vkapi_driver_bind_index_buffer(driver, engine->vbuffer->index_buffer);

    rpe_scene_t* scene = d->scene;
    assert(scene);
    rpe_render_queue_submit_one(scene->render_queue, driver, RPE_RENDER_QUEUE_DEPTH);

    vkapi_driver_end_rpass(cmd_buffer->instance);
}

rg_handle_t rpe_shadow_pass_render(
    rpe_shadow_manager_t* sm,
    render_graph_t* rg,
    rpe_scene_t* scene,
    uint32_t dimensions,
    VkFormat depth_format)
{
    assert(sm);
    assert(rg);

    struct ShadowLocalData local_d = {
        .prog_bundle = sm->csm_bundle,
        .width = dimensions,
        .height = dimensions,
        .depth_format = depth_format,
        .cascade_count = sm->settings.cascade_count,
        .scene = scene};
    rg_pass_t* p = rg_add_pass(
        rg,
        "ShadowPass",
        setup_shadow_pass,
        execute_shadow_pass,
        sizeof(struct ShadowPassData),
        &local_d);
    struct ShadowPassData* d = (struct ShadowPassData*)p->data;
    return d->depth;
}

void setup_cascade_debug_pass(
    render_graph_t* rg, rg_pass_node_t* node, void* data, void* local_data)
{
    struct CascadeDebugLocalData* local_d = (struct CascadeDebugLocalData*)local_data;
    struct CascadeDebugPassData* d = (struct CascadeDebugPassData*)data;
    rg_backboard_t* bb = rg_get_backboard(rg);
    rg_handle_t cascade_map = rg_backboard_get(bb, "CascadeShadowDepth");
    rg_handle_t light_colour = rg_backboard_get(bb, "light");

    rg_texture_desc_t t_desc = {
        .width = local_d->width,
        .height = local_d->height,
        .mip_levels = 1,
        .depth = 1,
        .layers = 1,
        .format = VK_FORMAT_R8G8B8A8_UNORM};

    d->colour = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "ShadowCascadeDebug", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);
    d->colour = rg_add_write(rg, d->colour, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->cascade_map = rg_add_read(rg, cascade_map, node, VK_IMAGE_USAGE_SAMPLED_BIT);
    d->light_colour = rg_add_read(rg, light_colour, node, VK_IMAGE_USAGE_SAMPLED_BIT);

    rg_pass_desc_t desc = rg_pass_desc_init();
    desc.attachments.attach.colour[0] = d->colour;
    desc.ds_load_clear_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    desc.ds_store_clear_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE;
    d->rt = rg_rpass_node_create_rt((rg_render_pass_node_t*)node, rg, "CascadeDebugPass", desc);

    rg_node_declare_side_effect((rg_node_t*)node);
    d->prog_bundle = local_d->prog_bundle;
}

void execute_cascade_debug_pass(
    vkapi_driver_t* driver, rpe_engine_t* engine, rg_render_graph_resource_t* res, void* data)
{
    struct CascadeDebugPassData* d = (struct CascadeDebugPassData*)data;
    rg_resource_info_t info = rg_res_get_render_pass_info(res, d->rt);
    struct Settings settings = engine->settings;
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);

    shader_bundle_add_image_sampler(
        d->prog_bundle, driver, rg_res_get_tex_handle(res, d->cascade_map), 0);
    shader_bundle_add_image_sampler(
        d->prog_bundle, driver, rg_res_get_tex_handle(res, d->light_colour), 1);

    vkapi_driver_begin_rpass(driver, cmd_buffer->instance, &info.data, &info.handle);
    vkapi_driver_bind_gfx_pipeline(driver, d->prog_bundle, false);
    vkapi_driver_set_push_constant(
        driver,
        d->prog_bundle,
        &settings.shadow.debug_cascade_idx,
        RPE_BACKEND_SHADER_STAGE_FRAGMENT);
    vkCmdDraw(cmd_buffer->instance, 3, 1, 0, 0);
    vkapi_driver_end_rpass(cmd_buffer->instance);
}

rg_handle_t rpe_cascade_shadow_debug_render(
    rpe_shadow_manager_t* sm, render_graph_t* rg, uint32_t width, uint32_t height)
{
    assert(sm);
    assert(rg);

    struct CascadeDebugLocalData local_d = {
        .prog_bundle = sm->csm_debug_bundle, .width = width, .height = height};
    rg_pass_t* p = rg_add_pass(
        rg,
        "CascadeDebugPass",
        setup_cascade_debug_pass,
        execute_cascade_debug_pass,
        sizeof(struct CascadeDebugPassData),
        &local_d);
    struct CascadeDebugPassData* d = (struct CascadeDebugPassData*)p->data;
    return d->colour;
}