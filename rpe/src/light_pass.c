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

#include "light_pass.h"

#include "engine.h"
#include "ibl.h"
#include "managers/light_manager.h"
#include "render_graph/render_graph.h"
#include "render_graph/render_pass_node.h"
#include "render_graph/rendergraph_resource.h"
#include "scene.h"

#include <backend/enums.h>
#include <vulkan-api/commands.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/sampler_cache.h>

void setup_light_pass(render_graph_t* rg, rg_pass_node_t* node, void* data, void* local_data)
{
    struct LightLocalData* local_d = (struct LightLocalData*)local_data;
    struct LightPassData* d = (struct LightPassData*)data;
    rg_backboard_t* bb = rg_get_backboard(rg);
    rpe_scene_t* scene = local_d->scene;

    // Get the resources from the colour pass
    rg_handle_t position = rg_backboard_get(bb, "position");
    rg_handle_t colour = rg_backboard_get(bb, "colour");
    rg_handle_t normal = rg_backboard_get(bb, "normal");
    rg_handle_t emissive = rg_backboard_get(bb, "emissive");
    rg_handle_t pbr = rg_backboard_get(bb, "pbr");

    rg_handle_t cascade_shadow_map;
    if (scene->shadow_status == RPE_SCENE_SHADOW_STATUS_ENABLED)
    {
        cascade_shadow_map = rg_backboard_get(bb, "CascadeShadowDepth");
    }

    rg_texture_desc_t t_desc = {
        .width = local_d->width,
        .height = local_d->height,
        .mip_levels = 1,
        .depth = 1,
        .layers = 1,
        .format = VK_FORMAT_R16G16B16A16_UNORM};
    d->light = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Light", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = local_d->depth_format;
    d->depth = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "LightDepth", VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    d->light = rg_add_write(rg, d->light, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->depth = rg_add_write(rg, d->depth, node, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // inputs into the pass
    d->position = rg_add_read(rg, position, node, VK_IMAGE_USAGE_SAMPLED_BIT);
    d->colour = rg_add_read(rg, colour, node, VK_IMAGE_USAGE_SAMPLED_BIT);
    d->normal = rg_add_read(rg, normal, node, VK_IMAGE_USAGE_SAMPLED_BIT);
    d->emissive = rg_add_read(rg, emissive, node, VK_IMAGE_USAGE_SAMPLED_BIT);
    d->pbr = rg_add_read(rg, pbr, node, VK_IMAGE_USAGE_SAMPLED_BIT);
    d->cascade_shadow_map.id = scene->shadow_status == RPE_SCENE_SHADOW_STATUS_ENABLED
        ? rg_add_read(rg, cascade_shadow_map, node, VK_IMAGE_USAGE_SAMPLED_BIT).id
        : UINT32_MAX;

    rg_backboard_add(bb, "light", d->light);
    rg_backboard_add(bb, "lightDepth", d->depth);

    rg_pass_desc_t desc = rg_pass_desc_init();
    desc.attachments.attach.colour[0] = d->light;
    desc.attachments.attach.depth = d->depth;
    desc.ds_load_clear_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    desc.ds_store_clear_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE;
    d->rt = rg_rpass_node_create_rt((rg_render_pass_node_t*)node, rg, "LightingPass", desc);

    d->prog_bundle = local_d->prog_bundle;
    d->scene = local_d->scene;
}

void execute_light_pass(
    vkapi_driver_t* driver, rpe_engine_t* engine, rg_render_graph_resource_t* res, void* data)
{
    struct LightPassData* d = (struct LightPassData*)data;
    rg_resource_info_t info = rg_res_get_render_pass_info(res, d->rt);

    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);

    // Use the GBuffer render targets as the samplers in this lighting pass.
    shader_bundle_add_image_sampler(
        d->prog_bundle,
        driver,
        rg_res_get_tex_handle(res, d->position),
        RPE_LIGHT_PASS_SAMPLER_POS_BINDING);
    shader_bundle_add_image_sampler(
        d->prog_bundle,
        driver,
        rg_res_get_tex_handle(res, d->colour),
        RPE_LIGHT_PASS_SAMPLER_COLOUR_BINDING);
    shader_bundle_add_image_sampler(
        d->prog_bundle,
        driver,
        rg_res_get_tex_handle(res, d->normal),
        RPE_LIGHT_PASS_SAMPLER_NORMAL_BINDING);
    shader_bundle_add_image_sampler(
        d->prog_bundle,
        driver,
        rg_res_get_tex_handle(res, d->pbr),
        RPE_LIGHT_PASS_SAMPLER_PBR_BINDING);
    shader_bundle_add_image_sampler(
        d->prog_bundle,
        driver,
        rg_res_get_tex_handle(res, d->emissive),
        RPE_LIGHT_PASS_SAMPLER_EMISSIVE_BINDING);

    // Shadow maps.
    texture_handle_t csm_handle;
    if (d->cascade_shadow_map.id != UINT32_MAX)
    {
        csm_handle = rg_res_get_tex_handle(res, d->cascade_shadow_map);
    }
    else
    {
        csm_handle = engine->tex_dummy_array;
    }
    shader_bundle_add_image_sampler(
        d->prog_bundle, driver, csm_handle, RPE_LIGHT_PASS_SAMPLER_CASCADE_SHADOW_MAP);


    // Bind the IBL env maps (dummy textures if not used to keep the validation layers happy).
    rpe_scene_t* scene = d->scene;
    assert(scene);
    ibl_t* ibl = scene->curr_ibl;
    texture_handle_t brdf_handle = ibl ? ibl->tex_brdf_lut : engine->tex_dummy;
    texture_handle_t irr_handle = ibl ? ibl->tex_irradiance_map : engine->tex_dummy_cubemap;
    texture_handle_t spec_handle = ibl ? ibl->tex_specular_map : engine->tex_dummy_cubemap;

    shader_bundle_add_image_sampler(
        d->prog_bundle, driver, brdf_handle, RPE_LIGHT_PASS_SAMPLER_BDRF_BINDING);
    shader_bundle_add_image_sampler(
        d->prog_bundle, driver, irr_handle, RPE_LIGHT_PASS_SAMPLER_IRRADIANCE_ENVMAP_BINDING);
    shader_bundle_add_image_sampler(
        d->prog_bundle, driver, spec_handle, RPE_LIGHT_PASS_SAMPLER_SPECULAR_ENVMAP_BINDING);

    vkapi_driver_begin_rpass(driver, cmd_buffer->instance, &info.data, &info.handle);
    vkapi_driver_draw_quad(driver, d->prog_bundle);
    vkapi_driver_end_rpass(cmd_buffer->instance);
}

rg_handle_t rpe_light_pass_render(
    rpe_light_manager_t* lm,
    render_graph_t* rg,
    rpe_scene_t* scene,
    uint32_t width,
    uint32_t height,
    VkFormat depth_format)
{
    assert(lm);
    assert(rg);

    // Seems like a good time to set the shadow specialised constant.
    lm->light_consts.draw_shadows =
        scene->shadow_status == RPE_SCENE_SHADOW_STATUS_ENABLED ? true : false;

    struct LightLocalData local_d = {
        .prog_bundle = lm->program_bundle,
        .width = width,
        .height = height,
        .depth_format = depth_format,
        .scene = scene};
    rg_pass_t* p = rg_add_pass(
        rg,
        "LightingPass",
        setup_light_pass,
        execute_light_pass,
        sizeof(struct LightPassData),
        &local_d);
    struct LightPassData* d = (struct LightPassData*)p->data;
    return d->light;
}
