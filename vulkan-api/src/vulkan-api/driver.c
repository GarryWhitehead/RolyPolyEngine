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

#include "driver.h"

#include "buffer.h"
#include "error_codes.h"
#include "frame_buffer_cache.h"
#include "program_manager.h"
#include "sampler_cache.h"
#include "swapchain.h"
#include "texture.h"
#include "utility.h"

#include <assert.h>
#include <string.h>
#include <utility/maths.h>

vkapi_driver_t* vkapi_driver_init(const char** instance_ext, uint32_t ext_count, int* error_code)
{
    *error_code = VKAPI_SUCCESS;

    // Allocate the arena space.
    arena_t perm_arena, scratch_arena;
    int perm_err = arena_new(VKAPI_PERM_ARENA_SIZE, &perm_arena);
    int scratch_err = arena_new(VKAPI_SCRATCH_ARENA_SIZE, &scratch_arena);
    if (perm_err != ARENA_SUCCESS || scratch_err != ARENA_SUCCESS)
    {
        *error_code = VKAPI_ERROR_INVALID_ARENA;
        return NULL;
    }

    vkapi_driver_t* driver = ARENA_MAKE_STRUCT(&perm_arena, vkapi_driver_t, ARENA_ZERO_MEMORY);
    assert(driver);
    driver->_perm_arena = perm_arena;
    driver->_scratch_arena = scratch_arena;

    VK_CHECK_RESULT(volkInitialize())
    driver->context = vkapi_context_init(&driver->_perm_arena);

    MAKE_DYN_ARRAY(vkapi_render_target_t, &driver->_perm_arena, 100, &driver->render_targets);

    // Create a new vulkan instance.
    *error_code = vkapi_context_create_instance(
        driver->context, instance_ext, ext_count, &driver->_perm_arena, &driver->_scratch_arena);
    if (*error_code != VKAPI_SUCCESS)
    {
        return NULL;
    }
    volkLoadInstance(driver->context->instance);
    return driver;
}

int vkapi_driver_create_device(vkapi_driver_t* driver, VkSurfaceKHR surface)
{
    assert(driver);
    // prepare the vulkan backend
    int ret = vkapi_context_prepare_device(driver->context, surface, &driver->_scratch_arena);
    if (ret != VKAPI_SUCCESS)
    {
        return ret;
    }

    driver->commands = vkapi_commands_init(driver->context, &driver->_perm_arena);

    // set up the memory allocator
    VmaVulkanFunctions vk_funcs = {0};
    vk_funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vk_funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo create_info = {0};
    create_info.vulkanApiVersion = VK_API_VERSION_1_2;
    create_info.physicalDevice = driver->context->physical;
    create_info.device = driver->context->device;
    create_info.instance = driver->context->instance;
    create_info.pVulkanFunctions = &vk_funcs;
    VkResult result = vmaCreateAllocator(&create_info, &driver->vma_allocator);
    assert(result == VK_SUCCESS);

    // create a semaphore for signaling that an image is ready for presentation
    VkSemaphoreCreateInfo sp_create_info = {0};
    sp_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateSemaphore(
        driver->context->device, &sp_create_info, VK_NULL_HANDLE, &driver->image_ready_signal))

    driver->prog_manager = program_cache_init(&driver->_perm_arena);
    driver->framebuffer_cache = vkapi_fb_cache_init(&driver->_perm_arena);
    driver->pline_cache = vkapi_pline_cache_init(&driver->_perm_arena, driver);
    driver->desc_cache = vkapi_desc_cache_init(driver, &driver->_perm_arena);
    driver->sampler_cache = vkapi_sampler_cache_init(&driver->_perm_arena);
    driver->res_cache = vkapi_res_cache_init(driver, &driver->_perm_arena);
    driver->staging_pool = vkapi_staging_init(&driver->_perm_arena);
    return VKAPI_SUCCESS;
}

void vkapi_driver_shutdown(vkapi_driver_t* driver)
{
    vkapi_fb_cache_destroy(driver->framebuffer_cache, driver);
    vkapi_pline_cache_destroy(driver->pline_cache);
    vkapi_desc_cache_destroy(driver->desc_cache);
    vkapi_res_cache_destroy(driver->res_cache, driver);
    vkapi_sampler_cache_destroy(driver->sampler_cache, driver);
    vkapi_commands_destroy(driver->context, driver->commands);
    vkapi_staging_destroy(driver->staging_pool, driver->vma_allocator);
    program_cache_destroy(driver->prog_manager, driver);

    vkDestroySemaphore(driver->context->device, driver->image_ready_signal, VK_NULL_HANDLE);
    vmaDestroyAllocator(driver->vma_allocator);

    vkapi_context_shutdown(driver->context);
    arena_release(&driver->_scratch_arena);
    arena_release(&driver->_perm_arena);
}

VkFormat vkapi_driver_get_supported_depth_format(vkapi_driver_t* driver)
{
    // in order of preference - TODO: allow user to define whether stencil
    // format is required or not
    VkFormat formats[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT};
    VkFormatFeatureFlags format_feature = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkFormat output = VK_FORMAT_UNDEFINED;
    for (int i = 0; i < 3; ++i)
    {
        VkFormatProperties* props = NULL;
        vkGetPhysicalDeviceFormatProperties(driver->context->physical, formats[i], props);
        if (format_feature == (props->optimalTilingFeatures & format_feature))
        {
            output = formats[i];
            break;
        }
    }
    return output;
}

vkapi_rt_handle_t vkapi_driver_create_rt(
    vkapi_driver_t* driver,
    bool multiView,
    math_vec4f clear_col,
    vkapi_attach_info_t colours[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT],
    vkapi_attach_info_t depth,
    vkapi_attach_info_t stencil)
{
    assert(driver);
    vkapi_render_target_t rt = {
        .depth = depth,
        .stencil = stencil,
        .clear_colour.r = clear_col.r,
        .clear_colour.g = clear_col.g,
        .clear_colour.b = clear_col.b,
        .clear_colour.a = clear_col.a,
        .samples = 1,
        .multi_view = multiView};
    memcpy(
        rt.colours,
        colours,
        sizeof(vkapi_attach_info_t) * VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT);

    vkapi_rt_handle_t handle = {.id = driver->render_targets.size};
    DYN_ARRAY_APPEND(&driver->render_targets, &rt);
    return handle;
}

void vkapi_driver_map_gpu_buffer(
    vkapi_driver_t* driver, buffer_handle_t h, size_t size, size_t offset, void* data)
{
    assert(data);
    vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, h);
    assert(buffer);
    vkapi_buffer_map_to_gpu_buffer(buffer, data, size, offset);
}

bool vkapi_driver_begin_frame(vkapi_driver_t* driver, vkapi_swapchain_t* sc)
{
    // get the next image index which will be the framebuffer we draw too
    VkResult res = vkAcquireNextImageKHR(
        driver->context->device,
        sc->sc_instance,
        UINT64_MAX,
        driver->image_ready_signal,
        VK_NULL_HANDLE,
        &driver->image_index);

    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
    {
        // TODO: Deal with window resize.
        return false;
    }

    assert(res == VK_SUCCESS);
    return true;
}

void vkapi_driver_end_frame(vkapi_driver_t* driver, vkapi_swapchain_t* sc)
{
    vkapi_commands_set_ext_wait_signal(driver->commands, driver->image_ready_signal);

    // submit the present cmd buffer and send to the queue
    vkapi_commands_flush(driver->context, driver->commands);
    VkSemaphore render_complete_signal = vkapi_commands_get_finished_signal(driver->commands);

    VkPresentInfoKHR pi = {0};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &render_complete_signal;
    pi.swapchainCount = 1;
    pi.pSwapchains = &sc->sc_instance;
    pi.pImageIndices = &driver->image_index;
    VK_CHECK_RESULT(vkQueuePresentKHR(driver->context->present_queue, &pi));

    log_debug(
        "KHR Presentation (image index: %i) - render wait signal: {%lu}",
        driver->image_index,
        render_complete_signal);

    // Destroy any resources that have reached their use by date.
    vkapi_driver_gc(driver);

    driver->current_frame++;
}

void vkapi_driver_begin_rpass(
    vkapi_driver_t* driver,
    VkCommandBuffer cmds,
    vkapi_render_pass_data_t* data,
    vkapi_rt_handle_t* rt_handle)
{
    assert(driver);
    assert(rt_handle->id < driver->render_targets.size);

    int attach_count = 0;
    vkapi_render_target_t* rt =
        DYN_ARRAY_GET_PTR(vkapi_render_target_t, &driver->render_targets, rt_handle->id);
    vkapi_attach_info_t depth = rt->depth;

    // Find a renderpass from the cache or create a new one.
    vkapi_rpass_key_t rpass_key = vkapi_rpass_key_init();
    rpass_key.depth = VK_FORMAT_UNDEFINED;
    if (vkapi_tex_handle_is_valid(depth.handle))
    {
        vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, rt->depth.handle);
        rpass_key.depth = tex->info.format;
        ++attach_count;
    }
    rpass_key.samples = rt->samples;
    rpass_key.multi_view = rt->multi_view;

    for (int i = 0; i < VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT; ++i)
    {
        vkapi_attach_info_t colour = rt->colours[i];
        rpass_key.colour_formats[i] = VK_FORMAT_UNDEFINED;
        if (vkapi_tex_handle_is_valid(colour.handle))
        {
            vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, colour.handle);
            rpass_key.colour_formats[i] = tex->info.format;
            assert(data->final_layouts[i] != VK_IMAGE_LAYOUT_UNDEFINED);
            rpass_key.final_layout[i] = data->final_layouts[i];
            rpass_key.initial_layout[i] = data->final_layouts[i];
            rpass_key.load_op[i] = data->load_clear_flags[i];
            rpass_key.store_op[i] = data->store_clear_flags[i];
            ++attach_count;
        }
    }
    rpass_key.ds_load_op[0] = data->load_clear_flags[VKAPI_RENDER_TARGET_DEPTH_INDEX - 1];
    rpass_key.ds_store_op[0] = data->store_clear_flags[VKAPI_RENDER_TARGET_DEPTH_INDEX - 1];
    rpass_key.ds_load_op[1] = data->load_clear_flags[VKAPI_RENDER_TARGET_STENCIL_INDEX - 1];
    rpass_key.ds_store_op[1] = data->store_clear_flags[VKAPI_RENDER_TARGET_STENCIL_INDEX - 1];

    vkapi_rpass_t* rpass = vkapi_fb_cache_find_or_create_rpass(
        driver->framebuffer_cache, &rpass_key, driver, &driver->_perm_arena);
    assert(rpass);

    // find a framebuffer from the cache or create a new one.
    vkapi_fbo_key_t fbo_key = vkapi_fbo_key_init();
    fbo_key.renderpass = rpass->instance;
    fbo_key.width = data->width;
    fbo_key.height = data->height;
    fbo_key.samples = rpass_key.samples;
    fbo_key.layer = 1;

    int count = 0;
    for (int idx = 0; idx < VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT; ++idx)
    {
        vkapi_attach_info_t colour = rt->colours[idx];
        if (vkapi_tex_handle_is_valid(colour.handle))
        {
            vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, colour.handle);
            fbo_key.views[idx] = tex->image_views[colour.level];
            assert(fbo_key.views[idx] && "ImageView for attachment is invalid.");
            ++count;
        }
    }
    if (vkapi_tex_handle_is_valid(rt->depth.handle))
    {
        vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, rt->depth.handle);
        fbo_key.views[count++] = tex->image_views[0];
    }

    vkapi_fbo_t* fbo =
        vkapi_fb_cache_find_or_create_fbo(driver->framebuffer_cache, &fbo_key, count, driver);
    fbo->last_used_frame_stamp = driver->current_frame;

    // Sort out the clear values for the pass - clear values required for each attachment.
    VkClearValue* clear_values = ARENA_MAKE_ARRAY(&driver->_scratch_arena, VkClearValue, attach_count, 0);

    for (int i = 0; i < attach_count; ++i)
    {
        VkAttachmentDescription* attach =
            DYN_ARRAY_GET_PTR(VkAttachmentDescription, &rpass->attach_descriptors, i);
        if (vkapi_util_is_depth(attach->format) || vkapi_util_is_stencil(attach->format))
        {
            clear_values[attach_count - 1].depthStencil.depth = 1.0f;
            clear_values[attach_count - 1].depthStencil.stencil = 0;
        }
        else
        {
            clear_values[i].color.float32[0] = rt->clear_colour.r;
            clear_values[i].color.float32[1] = rt->clear_colour.g;
            clear_values[i].color.float32[2] = rt->clear_colour.b;
            clear_values[i].color.float32[3] = rt->clear_colour.a;
        }
    }

    // extents of the frame buffer
    VkRect2D extents = {
        .offset.x = 0, .offset.y = 0, .extent.width = fbo->width, .extent.height = fbo->height};

    VkRenderPassBeginInfo bi = {0};
    bi.renderPass = rpass->instance;
    bi.framebuffer = fbo->instance;
    bi.renderArea = extents;
    bi.clearValueCount = attach_count;
    bi.pClearValues = clear_values;
    vkCmdBeginRenderPass(cmds, &bi, VK_SUBPASS_CONTENTS_INLINE);

    // Use a custom defined viewing area - at the moment set to the framebuffer
    // size
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)fbo->width,
        .height = (float)fbo->height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f};
    vkCmdSetViewport(cmds, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset.x = (int32_t)viewport.x,
        .offset.y = (int32_t)viewport.y,
        .extent.width = (int32_t)viewport.width,
        .extent.height = (int32_t)viewport.height};
    vkCmdSetScissor(cmds, 0, 1, &scissor);

    // bind the renderpass to the pipeline
    vkapi_pline_cache_bind_rpass(driver->pline_cache, rpass->instance);
    vkapi_pline_cache_bind_colour_attach_count(driver->pline_cache, rpass->attach_descriptors.size);
}

void vkapi_driver_end_rpass(VkCommandBuffer cmds) { vkCmdEndRenderPass(cmds); }

void vkapi_driver_bind_vertex_buffer(vkapi_driver_t* driver)
{
    // Map the vertex data first to the GPU.
    vkapi_res_cache_upload_vertex_buffer(driver->res_cache);

    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    VkDeviceSize offset[1] = {0};
    vkCmdBindVertexBuffers(
        cmd_buffer->instance, 0, 1, &driver->res_cache->vertex_buffer.buffer, offset);
}

void vkapi_driver_bind_index_buffer(vkapi_driver_t* driver)
{
    // Map the index data first to the GPU.
    vkapi_res_cache_upload_index_buffer(driver->res_cache);

    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkCmdBindIndexBuffer(
        cmd_buffer->instance, driver->res_cache->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}

void vkapi_driver_bind_gfx_pipeline(vkapi_driver_t* driver, shader_prog_bundle_t* bundle)
{
    vkapi_cmdbuffer_t* cmds = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkapi_pl_layout_t* pl_layout = vkapi_pline_cache_get_pl_layout(driver->pline_cache, bundle);

    // Bind all the buffers associated with this pipeline
    for (size_t i = 0; i < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT; ++i)
    {
        desc_bind_info_t* info = &bundle->ubos[i];
        vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, info->buffer);
        vkapi_desc_cache_bind_ubo(driver->desc_cache, info->binding, buffer->buffer, info->size);
    }
    for (size_t i = 0; i < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT; ++i)
    {
        desc_bind_info_t* info = &bundle->ssbos[i];
        vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, info->buffer);
        vkapi_desc_cache_bind_ssbo(driver->desc_cache, info->binding, buffer->buffer, info->size);
    }

    vkapi_desc_cache_bind_descriptors(
        driver->desc_cache,
        cmds->instance,
        bundle,
        pl_layout->instance,
        VK_PIPELINE_BIND_POINT_GRAPHICS);
    vkapi_pline_cache_bind_gfx_shader_modules(driver->pline_cache, bundle);

    // Bind the rasterisation and depth/stencil states
    vkapi_pline_cache_bind_cull_mode(driver->pline_cache, bundle->raster_state.cull_mode);
    vkapi_pline_cache_bind_front_face(driver->pline_cache, bundle->raster_state.front_face);
    vkapi_pline_cache_bind_polygon_mode(driver->pline_cache, bundle->raster_state.polygon_mode);

    // TODO: Need to support differences in front/back stencil
    VkPipelineDepthStencilStateCreateInfo ds_state = {0};
    ds_state.front.compareOp = bundle->ds_state.front.compare_op;
    ds_state.front.compareMask = bundle->ds_state.front.compare_mask;
    ds_state.front.depthFailOp = bundle->ds_state.front.depth_fail_op;
    ds_state.front.passOp = bundle->ds_state.front.pass_op;
    ds_state.front.reference = bundle->ds_state.front.reference;
    ds_state.front.failOp = bundle->ds_state.front.stencil_fail_op;
    ds_state.stencilTestEnable = bundle->ds_state.stencil_test_enable;
    vkapi_pline_cache_bind_depth_stencil_block(driver->pline_cache, &ds_state);

    // blend factors
    VkPipelineColorBlendAttachmentState blend_state = {0};
    blend_state.blendEnable = bundle->blend_state.blend_enable;
    blend_state.srcColorBlendFactor = bundle->blend_state.src_colour;
    blend_state.dstColorBlendFactor = bundle->blend_state.dst_colour;
    blend_state.colorBlendOp = bundle->blend_state.colour;
    blend_state.srcAlphaBlendFactor = bundle->blend_state.src_alpha;
    blend_state.dstAlphaBlendFactor = bundle->blend_state.dst_alpha;
    blend_state.alphaBlendOp = bundle->blend_state.alpha;
    vkapi_pline_cache_bind_blend_factor_block(driver->pline_cache, &blend_state);

    // Bind primitive info
    vkapi_pline_cache_bind_prim_restart(driver->pline_cache, bundle->render_prim.prim_restart);
    vkapi_pline_cache_bind_topology(driver->pline_cache, bundle->render_prim.topology);
    vkapi_pline_cache_bind_tess_vert_count(driver->pline_cache, bundle->tesse_vert_count);

    vkapi_pline_cache_bind_vertex_input(
        driver->pline_cache, bundle->vert_attrs, &bundle->vert_bind_desc);

    // if the width and height are zero then ignore setting the scissors and/or
    // viewport and go with the extents set upon initiation of the renderpass
    if (bundle->scissor.extent.width > 0 && bundle->scissor.extent.height > 0)
    {
        vkCmdSetScissor(cmds->instance, 0, 1, &bundle->scissor);
    }
    if (bundle->viewport.width > 0 && bundle->viewport.height > 0)
    {
        vkCmdSetViewport(cmds->instance, 0, 1, &bundle->viewport);
    }

    vkapi_pline_cache_bind_gfx_pl_layout(driver->pline_cache, pl_layout->instance);
    vkapi_pline_cache_bind_graphics_pline(
        driver->pline_cache, cmds->instance, bundle->spec_const_params);
}

void vkapi_driver_set_push_constant(
    vkapi_driver_t* driver, void* data, size_t size, VkShaderStageFlags stage)
{
    assert(driver);
    assert(data);
    assert(size > 0);
    VkPipelineLayout l = driver->pline_cache->graphics_pline_requires.pl_layout;
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkCmdPushConstants(cmd_buffer->instance, l, stage, 0, size, data);
}

void vkapi_driver_draw(vkapi_driver_t* driver, uint32_t vert_count, int32_t vertex_offset)
{
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkCmdDraw(cmd_buffer->instance, vert_count, 1, vertex_offset, 0);
}

void vkapi_driver_draw_indexed(
    vkapi_driver_t* driver, uint32_t index_count, int32_t vertex_offset, int32_t index_offset)
{
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkCmdDrawIndexed(cmd_buffer->instance, index_count, 1, index_offset, vertex_offset, 0);
}

void vkapi_driver_begin_cond_render(
    vkapi_driver_t* driver, buffer_handle_t cond_buffer, int32_t offset)
{
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, cond_buffer);
    VkConditionalRenderingBeginInfoEXT bi = {0};
    bi.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
    bi.buffer = buffer->buffer;
    bi.offset = offset;
    vkCmdBeginConditionalRenderingEXT(cmd_buffer->instance, &bi);
}

void vkapi_driver_dispatch_compute(
    vkapi_driver_t* driver,
    shader_prog_bundle_t* bundle,
    uint32_t x_work_count,
    uint32_t y_work_count,
    uint32_t z_work_count)
{
    vkapi_cmdbuffer_t* cmds = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkapi_pl_layout_t* pl_layout = vkapi_pline_cache_get_pl_layout(driver->pline_cache, bundle);

    // image storage
    struct DescriptorImage storage_images[VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT] = {0};
    for (int idx = 0; idx < VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT; ++idx)
    {
        texture_handle_t handle = bundle->storage_images[idx];
        if (vkapi_tex_handle_is_valid(handle))
        {
            vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, handle);
            storage_images[idx].image_view = tex->image_views[0];
            storage_images[idx].image_layout = tex->image_layout;
        }
    }
    vkapi_desc_cache_bind_storage_image(driver->desc_cache, storage_images);

    // image samplers
    struct DescriptorImage image_samplers[VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT] = {0};
    for (int idx = 0; idx < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT; ++idx)
    {
        texture_handle_t handle = bundle->storage_images[idx];
        VkSampler sampler = bundle->image_samplers[idx].sampler;
        if (vkapi_tex_handle_is_valid(handle))
        {
            vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, handle);
            image_samplers[idx].image_sampler = sampler;
            image_samplers[idx].image_view = tex->image_views[0];
            image_samplers[idx].image_layout = tex->image_layout;
        }
    }
    vkapi_desc_cache_bind_sampler(driver->desc_cache, image_samplers);

    // Bind all the buffers associated with this pipeline
    for (size_t i = 0; i < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT; ++i)
    {
        desc_bind_info_t* info = &bundle->ubos[i];
        if (vkapi_buffer_handle_is_valid(info->buffer))
        {
            vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, info->buffer);
            vkapi_desc_cache_bind_ubo(
                driver->desc_cache, info->binding, buffer->buffer, info->size);
        }
    }
    for (size_t i = 0; i < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT; ++i)
    {
        desc_bind_info_t* info = &bundle->ssbos[i];
        if (vkapi_buffer_handle_is_valid(info->buffer))
        {
            vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, info->buffer);
            vkapi_desc_cache_bind_ssbo(
                driver->desc_cache, info->binding, buffer->buffer, info->size);
        }
    }

    vkapi_desc_cache_bind_descriptors(
        driver->desc_cache,
        cmds->instance,
        bundle,
        pl_layout->instance,
        VK_PIPELINE_BIND_POINT_COMPUTE);
    vkapi_pline_cache_bind_compute_shader_modules(driver->pline_cache, bundle);

    vkapi_pline_cache_bind_compute_pl_layout(driver->pline_cache, pl_layout->instance);
    vkapi_pline_cache_bind_compute_pipeline(driver->pline_cache, cmds->instance);

    // Bind the push block.
    if (bundle->push_blocks[RPE_BACKEND_SHADER_STAGE_COMPUTE].range > 0)
    {
        struct PushBlockBindParams* params = &bundle->push_blocks[RPE_BACKEND_SHADER_STAGE_COMPUTE];
        assert(params->data);
        vkCmdPushConstants(
            cmds->instance,
            pl_layout->instance,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            params->range,
            params->data);
    }

    vkCmdDispatch(cmds->instance, x_work_count, y_work_count, z_work_count);
    vkapi_commands_flush(driver->context, driver->commands);
}

void vkapi_driver_gc(vkapi_driver_t* driver)
{
    vkapi_pline_cache_gc(driver->pline_cache, driver->current_frame);
    vkapi_desc_cache_gc(driver->desc_cache, driver->current_frame);
    vkapi_res_cache_gc(driver->res_cache, driver);
    vkapi_staging_gc(driver->staging_pool, driver->vma_allocator, driver->current_frame);
    vkapi_fb_cache_gc(driver->framebuffer_cache, driver, driver->current_frame);
}
