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

#include "colour_pass.h"

#include "engine.h"
#include "render_graph/render_graph.h"
#include "render_graph/render_pass_node.h"
#include "render_graph/rendergraph_resource.h"
#include "render_queue.h"
#include "scene.h"
#include "vertex_buffer.h"

#include <vulkan-api/driver.h>

void setup_gbuffer(render_graph_t* rg, rg_pass_node_t* node, void* data, void* local_data)
{
    struct GBufferLocalData* local_d = (struct GBufferLocalData*)local_data;
    struct DataGBuffer* d = (struct DataGBuffer*)data;
    rg_texture_desc_t t_desc = {
        .width = local_d->width, .height = local_d->height, .mip_levels = 1, .depth = 1, .layers = 1};

    t_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    d->colour = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Colour", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    d->pos = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Position", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    d->normal = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Normal", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = VK_FORMAT_R16G16_SFLOAT;
    d->pbr = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Pbr", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    d->emissive = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Emissive", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = local_d->depth_format;
    d->depth = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Depth", VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    d->colour = rg_add_write(rg, d->colour, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->pos = rg_add_write(rg, d->pos, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->normal = rg_add_write(rg, d->normal, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->pbr = rg_add_write(rg, d->pbr, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->emissive = rg_add_write(rg, d->emissive, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->depth = rg_add_write(rg, d->depth, node, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    rg_pass_desc_t desc = rg_pass_desc_init();
    desc.attachments.attach.colour[0] = d->colour;
    desc.attachments.attach.colour[1] = d->pos;
    desc.attachments.attach.colour[2] = d->normal;
    desc.attachments.attach.colour[3] = d->emissive;
    desc.attachments.attach.colour[4] = d->pbr;
    desc.attachments.attach.depth = d->depth;
    desc.ds_load_clear_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    desc.ds_load_clear_flags[1] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;

    d->rt = rg_rpass_node_create_rt((rg_render_pass_node_t*)node, rg, "GBufferPass", desc);
    rg_node_declare_side_effect((rg_node_t*)node);

    rg_backboard_t* bb = rg_get_backboard(rg);

    rg_backboard_add(bb, "colour", d->colour);
    rg_backboard_add(bb, "position", d->pos);
    rg_backboard_add(bb, "normal", d->normal);
    rg_backboard_add(bb, "emissive", d->emissive);
    rg_backboard_add(bb, "pbr", d->pbr);
    rg_backboard_add(bb, "gbufferDepth", d->depth);
}

void execute_gbuffer(
    vkapi_driver_t* driver, rpe_engine_t* engine, rg_render_graph_resource_t* res, void* data)
{
    struct DataGBuffer* d = (struct DataGBuffer*)data;
    rg_resource_info_t info = rg_res_get_render_pass_info(res, d->rt);

    vkapi_cmdbuffer_t* cmd_buffer = vkapi_driver_get_gfx_cmds(driver);
    rpe_vertex_buffer_upload_to_gpu(engine->vbuffer, driver);

    // Make sure the compute shaders have finished before commiting draw commands.
    vkapi_driver_acquire_buffer_barrier(
        driver,
        cmd_buffer,
        engine->curr_scene->indirect_draw_handle,
        VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ);
    vkapi_driver_acquire_buffer_barrier(
        driver,
        cmd_buffer,
        engine->curr_scene->draw_count_handle,
        VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ);

    vkapi_driver_begin_rpass(driver, cmd_buffer->instance, &info.data, &info.handle);

    // Bind the uber vertex/index buffers - only one bind call required as all draw calls offset
    // into this buffer.
    vkapi_driver_bind_vertex_buffer(driver, engine->vbuffer->vertex_buffer, 0);
    vkapi_driver_bind_vertex_buffer(driver, engine->curr_scene->model_draw_data_handle, 1);
    vkapi_driver_bind_index_buffer(driver, engine->vbuffer->index_buffer);

    rpe_scene_t* scene = engine->curr_scene;
    assert(scene && "No scene has been registered with the engine");
    rpe_render_queue_submit_one(scene->render_queue, driver, RPE_RENDER_QUEUE_GBUFFER);

    vkapi_driver_end_rpass(cmd_buffer->instance);

    vkapi_driver_release_buffer_barrier(
        driver,
        cmd_buffer,
        engine->curr_scene->indirect_draw_handle,
        VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ);
    vkapi_driver_release_buffer_barrier(
        driver,
        cmd_buffer,
        engine->curr_scene->draw_count_handle,
        VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ);
}

rg_handle_t
rpe_colour_pass_render(render_graph_t* rg, uint32_t dimensions, VkFormat depth_format)
{
    struct GBufferLocalData local_d = {
        .width = dimensions, .height = dimensions, .depth_format = depth_format};

    rg_pass_t* p = rg_add_pass(
        rg, "ColourPass", setup_gbuffer, execute_gbuffer, sizeof(struct DataGBuffer), &local_d);
    struct DataGBuffer* d = (struct DataGBuffer*)p->data;
    return d->colour;
}