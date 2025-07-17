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

vkapi_settings_t vkapi_driver_init_settings()
{
    vkapi_settings_t settings = {
        .threaded_cmdbuffers = true,
        .scratch_arena_size = VKAPI_SCRATCH_ARENA_SIZE,
        .perm_arena_size = VKAPI_PERM_ARENA_SIZE};
    return settings;
}

vkapi_driver_t* vkapi_driver_init(
    const char** instance_ext, uint32_t ext_count, vkapi_settings_t* settings, int* error_code)
{
    // RENDERDOC_CREATE_API_INSTANCE

    *error_code = VKAPI_SUCCESS;

    vkapi_settings_t s = !settings ? vkapi_driver_init_settings() : *settings;

    // Allocate the arena space.
    arena_t perm_arena, scratch_arena;
    int perm_err = arena_new(s.perm_arena_size, &perm_arena);
    int scratch_err = arena_new(s.scratch_arena_size, &scratch_arena);
    if (perm_err != ARENA_SUCCESS || scratch_err != ARENA_SUCCESS)
    {
        *error_code = VKAPI_ERROR_INVALID_ARENA;
        return NULL;
    }

    vkapi_driver_t* driver = ARENA_MAKE_STRUCT(&perm_arena, vkapi_driver_t, ARENA_ZERO_MEMORY);
    assert(driver);
    driver->_perm_arena = perm_arena;
    driver->_scratch_arena = scratch_arena;
    driver->settings = s;

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

    volkLoadDevice(driver->context->device);
    driver->commands = vkapi_commands_init(
        driver->context,
        driver->context->queue_info.graphics,
        driver->context->graphics_queue,
        &driver->_perm_arena);
    driver->compute_commands = vkapi_commands_init(
        driver->context,
        driver->context->queue_info.compute,
        driver->context->compute_queue,
        &driver->_perm_arena);

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

    // The staging pool is needed by some of the other caches so init first.
    driver->staging_pool = vkapi_staging_init(&driver->_perm_arena);
    driver->prog_manager = program_cache_init(&driver->_perm_arena);
    driver->framebuffer_cache = vkapi_fb_cache_init(&driver->_perm_arena);
    driver->pline_cache = vkapi_pline_cache_init(&driver->_perm_arena, driver);
    driver->desc_cache = vkapi_desc_cache_init(driver, &driver->_perm_arena);
    driver->sampler_cache = vkapi_sampler_cache_init(&driver->_perm_arena);
    driver->res_cache = vkapi_res_cache_init(driver, &driver->_perm_arena);

    if (driver->settings.threaded_cmdbuffers)
    {
        for (int idx = 0; idx < VKAPI_COMMANDS_MAX_THREAD_COUNT; ++idx)
        {
            driver->thread_draw_info[idx].desc_cache =
                vkapi_desc_cache_init(driver, &driver->_perm_arena);
            driver->thread_draw_info[idx].pline_cache =
                vkapi_pline_cache_init(&driver->_perm_arena, driver);
        }
    }

    return VKAPI_SUCCESS;
}

void vkapi_driver_shutdown(vkapi_driver_t* driver, VkSurfaceKHR surface)
{
    // Destroy the command buffers first to make sure they have executed all commands before
    // destroying all other Vulkan objects.
    vkapi_commands_destroy(driver->context, driver->commands);
    vkapi_commands_destroy(driver->context, driver->compute_commands);

    if (driver->settings.threaded_cmdbuffers)
    {
        for (int i = 0; i < VKAPI_COMMANDS_MAX_THREAD_COUNT; ++i)
        {
            vkapi_pline_cache_destroy(driver->thread_draw_info[i].pline_cache);
            vkapi_desc_cache_destroy(driver->thread_draw_info[i].desc_cache);
        }
    }

    vkapi_fb_cache_destroy(driver->framebuffer_cache, driver);
    vkapi_pline_cache_destroy(driver->pline_cache);
    vkapi_desc_cache_destroy(driver->desc_cache);
    vkapi_res_cache_destroy(driver->res_cache, driver);
    vkapi_sampler_cache_destroy(driver->sampler_cache, driver);
    vkapi_staging_destroy(driver->staging_pool, driver->vma_allocator);
    program_cache_destroy(driver->prog_manager, driver);

    vkDestroySemaphore(driver->context->device, driver->image_ready_signal, VK_NULL_HANDLE);
    vmaDestroyAllocator(driver->vma_allocator);

    vkapi_context_shutdown(driver->context, surface);
    arena_release(&driver->_scratch_arena);
    arena_release(&driver->_perm_arena);
}

VkFormat vkapi_driver_get_supported_depth_format(vkapi_driver_t* driver)
{
    VkFormat formats[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT};

    VkFormat output = VK_FORMAT_UNDEFINED;
    for (int i = 0; i < 3; ++i)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(driver->context->physical, formats[i], &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            output = formats[i];
            break;
        }
    }
    return output;
}

vkapi_rt_handle_t vkapi_driver_create_rt(
    vkapi_driver_t* driver,
    uint32_t multi_view_count,
    math_vec4f clear_col,
    vkapi_attach_info_t* colours,
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
        .multi_view_count = multi_view_count};
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
    assert(vkapi_buffer_handle_is_valid(h));
    vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, h);
    assert(buffer);
    vkapi_buffer_map_to_gpu_buffer(buffer, data, size, offset);
}

void vkapi_driver_destroy_rt(vkapi_driver_t* driver, vkapi_rt_handle_t* h)
{
    assert(driver);
    assert(h->id < driver->render_targets.size);
    DYN_ARRAY_REMOVE(&driver->render_targets, h->id);
    h->id = UINT32_MAX;
}

void vkapi_driver_map_gpu_texture(
    vkapi_driver_t* driver,
    texture_handle_t h,
    void* data,
    size_t sz,
    size_t* offsets,
    bool generate_mipmaps)
{
    assert(data);
    assert(vkapi_tex_handle_is_valid(h));
    vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, h);
    vkapi_texture_map(
        driver->context,
        driver->staging_pool,
        driver->commands,
        driver->vma_allocator,
        tex,
        data,
        sz,
        offsets,
        &driver->_scratch_arena,
        generate_mipmaps);
}

void vkapi_driver_map_gpu_vertex(
    vkapi_driver_t* driver,
    buffer_handle_t vertex_handle,
    void* vertices,
    uint32_t vertex_sz,
    buffer_handle_t index_handle,
    void* indices,
    uint32_t indices_sz)
{
    vkapi_buffer_t* vert_buffer = vkapi_res_cache_get_buffer(driver->res_cache, vertex_handle);
    vkapi_buffer_t* index_buffer = vkapi_res_cache_get_buffer(driver->res_cache, index_handle);

    vkapi_buffer_upload_vertex_data(vert_buffer, driver, vertices, vertex_sz, 0);
    vkapi_buffer_upload_index_data(index_buffer, driver, indices, indices_sz, 0);
}

bool vkapi_driver_begin_frame(vkapi_driver_t* driver, vkapi_swapchain_t* sc)
{
    assert(driver);
    assert(sc);

    // CLear the secondary cmd buffers ready for recording.
    vkapi_commands_new_frame_sc(driver->commands);

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
    assert(driver);
    assert(sc);

    vkapi_driver_flush_compute_cmds(driver);

    vkapi_commands_set_ext_wait_signal(driver->commands, driver->image_ready_signal);
    vkapi_commands_set_ext_wait_signal(
        driver->commands, vkapi_commands_get_finished_signal(driver->compute_commands));

    // submit the present cmd buffer and send to the queue
    vkapi_driver_flush_gfx_cmds(driver);
    VkSemaphore render_complete_signal = vkapi_commands_get_finished_signal(driver->commands);

    VkPresentInfoKHR pi = {0};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &render_complete_signal;
    pi.swapchainCount = 1;
    pi.pSwapchains = &sc->sc_instance;
    pi.pImageIndices = &driver->image_index;
    VK_CHECK_RESULT(vkQueuePresentKHR(driver->context->present_queue, &pi))

    // Destroy any resources that have reached their use by date.
    vkapi_driver_gc(driver);

    driver->current_frame++;
}

void vkapi_driver_begin_rpass(
    vkapi_driver_t* driver, vkapi_render_pass_data_t* data, vkapi_rt_handle_t* rt_handle, bool use_threads)
{
    assert(driver);
    assert(rt_handle->id < driver->render_targets.size);

    vkapi_cmdbuffer_t* cmd_buffer = vkapi_driver_get_gfx_cmds(driver);
    VkCommandBuffer cmds = cmd_buffer->instance;

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
    rpass_key.multi_view_count = rt->multi_view_count;

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
            rpass_key.initial_layout[i] = data->init_layouts[i];
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

            fbo_key.views[idx] =
                colour.level > 0 ? tex->image_views[colour.level] : tex->framebuffer_imageview;
            assert(fbo_key.views[idx] && "ImageView for attachment is invalid.");
            ++count;
        }
    }
    if (vkapi_tex_handle_is_valid(rt->depth.handle))
    {
        vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, rt->depth.handle);
        fbo_key.views[count++] = tex->framebuffer_imageview;
    }

    vkapi_fbo_t* fbo =
        vkapi_fb_cache_find_or_create_fbo(driver->framebuffer_cache, &fbo_key, count, driver);
    fbo->last_used_frame_stamp = driver->current_frame;

    // Sort out the clear values for the pass - clear values required for each attachment.
    VkClearValue* clear_values =
        ARENA_MAKE_ARRAY(&driver->_scratch_arena, VkClearValue, attach_count, 0);

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

    bool threaded_cmdbuffers = driver->settings.threaded_cmdbuffers & use_threads;

    // extents of the frame buffer
    VkRect2D extents = {
        .offset.x = 0, .offset.y = 0, .extent.width = fbo->width, .extent.height = fbo->height};

    VkRenderPassBeginInfo bi = {0};
    bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    bi.renderPass = rpass->instance;
    bi.framebuffer = fbo->instance;
    bi.renderArea = extents;
    bi.clearValueCount = attach_count;
    bi.pClearValues = clear_values;
    vkCmdBeginRenderPass(
        cmds,
        &bi,
        threaded_cmdbuffers ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
                            : VK_SUBPASS_CONTENTS_INLINE);

    vkapi_commands_set_rpass_sc(driver->commands, rpass->instance, fbo->instance);

    VkRect2D scissor = {.extent.width = fbo->width, .extent.height = fbo->height };
    VkViewport vp = { .width = (float)fbo->width, .height = (float)fbo->height, .maxDepth = 1.0f};
    vkapi_commands_set_viewport_scissor_state(driver->commands, scissor, vp);

    // bind the renderpass to the pipeline
    if (threaded_cmdbuffers)
    {
        for (int i = 0; i < VKAPI_COMMANDS_MAX_THREAD_COUNT; ++i)
        {
            vkapi_pipeline_cache_t* cache = driver->thread_draw_info[i].pline_cache;
            vkapi_pline_cache_bind_rpass(cache, rpass->instance);
            vkapi_pline_cache_bind_colour_attach_count(cache, vkapi_rpass_get_attach_count(rpass));
        }
    }
    else
    {
        vkapi_pline_cache_bind_rpass(driver->pline_cache, rpass->instance);
        vkapi_pline_cache_bind_colour_attach_count(
            driver->pline_cache, vkapi_rpass_get_attach_count(rpass));
    }
    arena_reset(&driver->_scratch_arena);
}

void vkapi_driver_end_rpass(VkCommandBuffer cmds) { vkCmdEndRenderPass(cmds); }

void _bind_vertex_buffer(
    vkapi_driver_t* driver, buffer_handle_t vb_handle, VkCommandBuffer cmds, uint32_t binding)
{
    vkapi_buffer_t* vb = vkapi_res_cache_get_buffer(driver->res_cache, vb_handle);
    VkDeviceSize offset[1] = {0};
    vkCmdBindVertexBuffers(cmds, binding, 1, &vb->buffer, offset);
}

void vkapi_driver_bind_vertex_buffer_sc(
    vkapi_driver_t* driver, buffer_handle_t vb_handle, uint32_t binding, uint32_t id)
{
    assert(driver);
    assert(vkapi_buffer_handle_is_valid(vb_handle));
    VkCommandBuffer cmds = vkapi_commands_get_secondary_cmds(driver->commands, id);
    _bind_vertex_buffer(driver, vb_handle, cmds, binding);
}

void vkapi_driver_bind_vertex_buffer(
    vkapi_driver_t* driver, buffer_handle_t vb_handle, uint32_t binding)
{
    assert(driver);
    assert(vkapi_buffer_handle_is_valid(vb_handle));
    VkCommandBuffer cmds = vkapi_driver_get_gfx_cmds(driver)->instance;
    _bind_vertex_buffer(driver, vb_handle, cmds, binding);
}

void _bind_index_buffer(vkapi_driver_t* driver, buffer_handle_t ib_handle, VkCommandBuffer cmds)
{
    vkapi_buffer_t* ib = vkapi_res_cache_get_buffer(driver->res_cache, ib_handle);
    vkCmdBindIndexBuffer(cmds, ib->buffer, 0, VK_INDEX_TYPE_UINT32);
}

void vkapi_driver_bind_index_buffer_sc(
    vkapi_driver_t* driver, buffer_handle_t ib_handle, uint32_t id)
{
    assert(driver);
    assert(vkapi_buffer_handle_is_valid(ib_handle));
    VkCommandBuffer cmds = vkapi_commands_get_secondary_cmds(driver->commands, id);
    _bind_index_buffer(driver, ib_handle, cmds);
}

void vkapi_driver_bind_index_buffer(vkapi_driver_t* driver, buffer_handle_t ib_handle)
{
    assert(driver);
    assert(vkapi_buffer_handle_is_valid(ib_handle));
    VkCommandBuffer cmds = vkapi_driver_get_gfx_cmds(driver)->instance;
    _bind_index_buffer(driver, ib_handle, cmds);
}

void _bind_descriptors(
    vkapi_driver_t* driver,
    vkapi_desc_cache_t* desc_cache,
    vkapi_pipeline_cache_t* pline_cache,
    shader_prog_bundle_t* bundle,
    VkCommandBuffer cmds,
    bool force_rebind)
{
    assert(driver);
    assert(desc_cache);
    assert(pline_cache);

    vkapi_pl_layout_t* pl_layout = vkapi_pline_cache_get_pl_layout(pline_cache, bundle);

    bool bound_samplers = false;
    struct DescriptorImage image_samplers[VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT] = {0};
    for (int idx = 0; idx < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT; ++idx)
    {
        texture_handle_t handle = bundle->image_samplers[idx].handle;
        VkSampler sampler = bundle->image_samplers[idx].sampler;
        if (vkapi_tex_handle_is_valid(handle))
        {
            vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, handle);
            image_samplers[idx].image_sampler = sampler;
            image_samplers[idx].image_view = tex->image_views[0];
            image_samplers[idx].image_layout = tex->image_layout;
            bound_samplers = true;
        }
    }
    if (bound_samplers)
    {
        vkapi_desc_cache_bind_sampler(desc_cache, image_samplers);
    }

    // Bind all the buffers associated with this pipeline
    for (size_t i = 0; i < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT; ++i)
    {
        desc_bind_info_t* info = &bundle->ubos[i];
        if (vkapi_buffer_handle_is_valid(info->buffer))
        {
            vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, info->buffer);
            vkapi_desc_cache_bind_ubo(desc_cache, info->binding, buffer->buffer, info->size);
        }
    }
    for (size_t i = 0; i < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT; ++i)
    {
        desc_bind_info_t* info = &bundle->ssbos[i];
        if (vkapi_buffer_handle_is_valid(info->buffer))
        {
            vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, info->buffer);
            vkapi_desc_cache_bind_ssbo(desc_cache, info->binding, buffer->buffer, info->size);
        }
    }

    vkapi_desc_cache_bind_descriptors(
        desc_cache,
        cmds,
        bundle,
        pl_layout->instance,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        force_rebind);

    vkapi_pline_cache_bind_gfx_pl_layout(pline_cache, pl_layout->instance);
}

void vkapi_driver_bind_descriptors_sc(
    vkapi_driver_t* driver, shader_prog_bundle_t* bundle, bool force_rebind, uint32_t id)
{
    assert(driver);

    uint32_t thread_id = id % VKAPI_COMMANDS_MAX_THREAD_COUNT;
    vkapi_pipeline_cache_t* pline_cache = driver->thread_draw_info[thread_id].pline_cache;
    vkapi_desc_cache_t* desc_cache = driver->thread_draw_info[thread_id].desc_cache;
    VkCommandBuffer cmd_buffer = vkapi_commands_get_secondary_cmds(driver->commands, id);

    _bind_descriptors(driver, desc_cache, pline_cache, bundle, cmd_buffer, force_rebind);
}

void vkapi_driver_bind_descriptors(
    vkapi_driver_t* driver, shader_prog_bundle_t* bundle, bool force_rebind)
{
    assert(driver);
    VkCommandBuffer cmd_buffer = vkapi_driver_get_gfx_cmds(driver)->instance;
    _bind_descriptors(
        driver, driver->desc_cache, driver->pline_cache, bundle, cmd_buffer, force_rebind);
}

void _bind_gfx_pipeline(
    VkCommandBuffer cmds,
    vkapi_pipeline_cache_t* pline_cache,
    shader_prog_bundle_t* bundle)
{
    assert(bundle);
    assert(pline_cache);
    assert(
        pline_cache->graphics_pline_requires.pl_layout &&
        "Pipeline layout not set; Have you called `bind_descriptors` before calling this "
        "function?");

    vkapi_pline_cache_bind_gfx_shader_modules(pline_cache, bundle);

    // Bind the rasterisation and depth/stencil states
    vkapi_pline_cache_bind_cull_mode(pline_cache, bundle->raster_state.cull_mode);
    vkapi_pline_cache_bind_front_face(pline_cache, bundle->raster_state.front_face);
    vkapi_pline_cache_bind_polygon_mode(pline_cache, bundle->raster_state.polygon_mode);
    vkapi_pline_cache_bind_depth_test_enable(pline_cache, bundle->ds_state.test_enable);
    vkapi_pline_cache_bind_depth_write_enable(pline_cache, bundle->ds_state.write_enable);
    vkapi_pline_cache_bind_depth_compare_op(pline_cache, bundle->ds_state.compare_op);
    vkapi_pline_cache_bind_depth_clamp(pline_cache, bundle->raster_state.depth_clamp_enable);

    // TODO: Need to support differences in front/back stencil
    struct DepthStencilBlock ds_state = {0};
    ds_state.compare_op = bundle->ds_state.front.compare_op;
    ds_state.compare_mask = bundle->ds_state.front.compare_mask;
    ds_state.depth_fail_op = bundle->ds_state.front.depth_fail_op;
    ds_state.pass_op = bundle->ds_state.front.pass_op;
    ds_state.reference = bundle->ds_state.front.reference;
    ds_state.stencil_fail_op = bundle->ds_state.front.stencil_fail_op;
    ds_state.stencil_test_enable = bundle->ds_state.stencil_test_enable;
    vkapi_pline_cache_bind_depth_stencil_block(pline_cache, &ds_state);

    // blend factors
    struct BlendFactorBlock blend_state = {0};
    blend_state.blend_enable = bundle->blend_state.blend_enable;
    blend_state.src_colour_blend_factor = bundle->blend_state.src_colour;
    blend_state.dst_colour_blend_factor = bundle->blend_state.dst_colour;
    blend_state.colour_blend_op = bundle->blend_state.colour;
    blend_state.src_alpha_blend_factor = bundle->blend_state.src_alpha;
    blend_state.dst_alpha_blend_factor = bundle->blend_state.dst_alpha;
    blend_state.alpha_blend_op = bundle->blend_state.alpha;
    vkapi_pline_cache_bind_blend_factor_block(pline_cache, &blend_state);

    // Bind primitive info
    vkapi_pline_cache_bind_prim_restart(pline_cache, bundle->render_prim.prim_restart);
    vkapi_pline_cache_bind_topology(pline_cache, bundle->render_prim.topology);
    vkapi_pline_cache_bind_tess_vert_count(pline_cache, bundle->tesse_vert_count);

    vkapi_pline_cache_bind_vertex_input(pline_cache, bundle->vert_attrs, bundle->vert_bind_desc);
    vkapi_pline_cache_bind_spec_constants(pline_cache, bundle);

    vkapi_pline_cache_bind_graphics_pline(
        pline_cache, cmds, bundle->spec_const_params);
}

void vkapi_driver_bind_gfx_pipeline_sc(
    vkapi_driver_t* driver, shader_prog_bundle_t* bundle, uint32_t id)
{
    assert(driver);

    uint32_t thread_id = id % VKAPI_COMMANDS_MAX_THREAD_COUNT;
    vkapi_pipeline_cache_t* pline_cache = driver->thread_draw_info[thread_id].pline_cache;
    VkCommandBuffer cmd_buffer = vkapi_commands_get_secondary_cmds(driver->commands, id);

    _bind_gfx_pipeline(cmd_buffer, pline_cache, bundle);
}

void vkapi_driver_bind_gfx_pipeline(
    vkapi_driver_t* driver, shader_prog_bundle_t* bundle)
{
    assert(driver);
    VkCommandBuffer cmd_buffer = vkapi_driver_get_gfx_cmds(driver)->instance;
    _bind_gfx_pipeline(cmd_buffer, driver->pline_cache, bundle);
}

void _set_scissor(vkapi_driver_t* driver, VkRect2D scissor, VkCommandBuffer cmds)
{
    vkCmdSetScissor(cmds, 0, 1, &scissor);
}

void vkapi_driver_set_scissor_sc(vkapi_driver_t* driver, VkRect2D scissor, uint32_t id)
{
    assert(driver);
    VkCommandBuffer cmd_buffer = vkapi_commands_get_secondary_cmds(driver->commands, id);
    _set_scissor(driver, scissor, cmd_buffer);
}

void vkapi_driver_set_scissor(vkapi_driver_t* driver, VkRect2D scissor)
{
    assert(driver);
    VkCommandBuffer cmd_buffer = vkapi_driver_get_gfx_cmds(driver)->instance;
    _set_scissor(driver, scissor, cmd_buffer);
}

void _set_viewport(vkapi_driver_t* driver, VkViewport vp, VkCommandBuffer cmds)
{
    vkCmdSetViewport(cmds, 0, 1, &vp);
}

void vkapi_driver_set_viewport_sc(vkapi_driver_t* driver, VkViewport vp, uint32_t id)
{
    assert(driver);
    VkCommandBuffer cmd_buffer = vkapi_commands_get_secondary_cmds(driver->commands, id);
    _set_viewport(driver, vp, cmd_buffer);
}

void vkapi_driver_set_viewport(vkapi_driver_t* driver, VkViewport vp)
{
    assert(driver);
    VkCommandBuffer cmd_buffer = vkapi_driver_get_gfx_cmds(driver)->instance;
    _set_viewport(driver, vp, cmd_buffer);
}

void vkapi_driver_set_push_constant(
    vkapi_driver_t* driver, shader_prog_bundle_t* bundle, void* data, enum ShaderStage stage)
{
    assert(driver);
    assert(data);
    VkPipelineLayout l = driver->pline_cache->bound_graphics_pline.pl_layout;
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    size_t size = bundle->push_blocks[stage].range;
    assert(size > 0);
    vkCmdPushConstants(cmd_buffer->instance, l, bundle->push_blocks[stage].stage, 0, size, data);
}

void vkapi_driver_draw(vkapi_driver_t* driver, uint32_t vert_count, int32_t vertex_offset)
{
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkCmdDraw(cmd_buffer->instance, vert_count, 1, vertex_offset, 0);
}

void vkapi_driver_draw_indexed(
    vkapi_driver_t* driver, uint32_t index_count, int32_t vertex_offset, int32_t index_offset)
{
    assert(driver);
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkCmdDrawIndexed(cmd_buffer->instance, index_count, 1, index_offset, vertex_offset, 0);
}

void _draw_indirect_indexed(
    vkapi_driver_t* driver,
    buffer_handle_t indirect_cmd_buffer,
    uint32_t offset,
    buffer_handle_t cmd_count_buffer,
    uint32_t draw_count_offset,
    uint32_t stride,
    uint32_t max_draw_count,
    VkCommandBuffer cmds)
{
    assert(driver);
    assert(max_draw_count > 0);
    assert(stride > 0);
    assert(vkapi_buffer_handle_is_valid(indirect_cmd_buffer));
    assert(vkapi_buffer_handle_is_valid(cmd_count_buffer));

    vkapi_buffer_t* ic_buffer = vkapi_res_cache_get_buffer(driver->res_cache, indirect_cmd_buffer);
    vkapi_buffer_t* count_buffer = vkapi_res_cache_get_buffer(driver->res_cache, cmd_count_buffer);
    vkCmdDrawIndexedIndirectCount(
        cmds,
        ic_buffer->buffer,
        offset,
        count_buffer->buffer,
        draw_count_offset,
        max_draw_count,
        stride);
}

void vkapi_driver_draw_indirect_indexed_sc(
    vkapi_driver_t* driver,
    buffer_handle_t indirect_cmd_buffer,
    uint32_t offset,
    buffer_handle_t cmd_count_buffer,
    uint32_t draw_count_offset,
    uint32_t stride,
    uint32_t max_draw_count,
    uint32_t id)
{
    assert(driver);
    assert(vkapi_buffer_handle_is_valid(indirect_cmd_buffer));
    assert(vkapi_buffer_handle_is_valid(cmd_count_buffer));
    VkCommandBuffer cmds = vkapi_commands_get_secondary_cmds(driver->commands, id);
    _draw_indirect_indexed(
        driver, indirect_cmd_buffer, offset, cmd_count_buffer, draw_count_offset, stride, max_draw_count, cmds);
}

void vkapi_driver_draw_indirect_indexed(
    vkapi_driver_t* driver,
    buffer_handle_t indirect_cmd_buffer,
    uint32_t offset,
    buffer_handle_t cmd_count_buffer,
    uint32_t draw_count_offset,
    uint32_t stride,
    uint32_t max_draw_count)
{
    assert(driver);
    assert(vkapi_buffer_handle_is_valid(indirect_cmd_buffer));
    assert(vkapi_buffer_handle_is_valid(cmd_count_buffer));
    VkCommandBuffer cmds = vkapi_driver_get_gfx_cmds(driver)->instance;
    _draw_indirect_indexed(
        driver, indirect_cmd_buffer, offset, cmd_count_buffer, draw_count_offset, stride, max_draw_count, cmds);
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
    vkapi_cmdbuffer_t* cmds = vkapi_driver_get_compute_cmds(driver);
    vkapi_pl_layout_t* pl_layout = vkapi_pline_cache_get_pl_layout(driver->pline_cache, bundle);

    VkPipelineLayout pl_instance = pl_layout->instance;

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
        texture_handle_t handle = bundle->image_samplers[idx].handle;
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

    RENDERDOC_START_CAPTURE(NULL, NULL)
    vkapi_desc_cache_bind_descriptors(
        driver->desc_cache,
        cmds->instance,
        bundle,
        pl_instance,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        false);
    vkapi_pline_cache_bind_compute_shader_modules(driver->pline_cache, bundle);

    vkapi_pline_cache_bind_compute_pl_layout(driver->pline_cache, pl_instance);
    vkapi_pline_cache_bind_compute_pipeline(driver->pline_cache, cmds->instance);
    RENDERDOC_END_CAPTURE(NULL, NULL)

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

    RENDERDOC_START_CAPTURE(NULL, NULL)
    vkCmdDispatch(cmds->instance, x_work_count, y_work_count, z_work_count);
    RENDERDOC_END_CAPTURE(NULL, NULL)
}

void vkapi_driver_draw_quad(vkapi_driver_t* driver, shader_prog_bundle_t* bundle, VkRect2D scissor, VkViewport vp)
{
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkapi_driver_bind_descriptors(driver, bundle, true);
    vkapi_driver_bind_gfx_pipeline(driver, bundle);
    vkapi_driver_set_scissor(driver, scissor);
    vkapi_driver_set_viewport(driver, vp);

    vkCmdDraw(cmd_buffer->instance, 3, 1, 0, 0);
}

void vkapi_driver_generate_mipmaps(
    vkapi_driver_t* driver, texture_handle_t handle, size_t level_count)
{
    assert(driver);
    vkapi_texture_t* tex = vkapi_res_cache_get_tex2d(driver->res_cache, handle);
    vkapi_texture_gen_mipmaps(tex, driver->context, driver->commands, level_count);
}

void vkapi_driver_transition_image(
    vkapi_driver_t* driver,
    texture_handle_t h,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    uint32_t mip_levels)
{
    if (old_layout == new_layout)
    {
        return;
    }

    vkapi_cmdbuffer_t* cmds = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    vkapi_texture_t* texture = vkapi_res_cache_get_tex2d(driver->res_cache, h);

    VkPipelineStageFlags old_flags = vkapi_texture_get_pline_stage_flag(old_layout);
    VkPipelineStageFlags new_flags = vkapi_texture_get_pline_stage_flag(new_layout);

    vkapi_texture_image_multi_transition(
        texture, old_layout, new_layout, cmds->instance, old_flags, new_flags, mip_levels);
}

void vkapi_driver_apply_global_barrier(
    vkapi_driver_t* driver,
    VkPipelineStageFlags src_stage,
    VkPipelineStageFlags dst_stage,
    VkAccessFlags src_access,
    VkAccessFlags dst_access)
{
    assert(driver);
    vkapi_cmdbuffer_t* cmd_buffer = vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
    VkMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = src_access,
        .dstAccessMask = dst_access};
    vkCmdPipelineBarrier(
        cmd_buffer->instance,
        src_stage,
        dst_stage,
        0,
        1,
        &barrier,
        0,
        VK_NULL_HANDLE,
        0,
        VK_NULL_HANDLE);
}

void vkapi_driver_acquire_buffer_barrier(
    vkapi_driver_t* driver,
    vkapi_cmdbuffer_t* cmd_buffer,
    buffer_handle_t handle,
    enum BarrierType type)
{
    // No sync required if the graphics and compute queues are the same.
    if (driver->context->queue_info.graphics == driver->context->queue_info.compute)
    {
        return;
    }

    vkapi_buffer_t* b = vkapi_res_cache_get_buffer(driver->res_cache, handle);

    VkPipelineStageFlags srcStage, dstStage;
    VkAccessFlags srcAccess, dstAccess;
    uint32_t srcQueueIndex, dstQueueIndex;

    if (type == VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ)
    {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        dstStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        srcAccess = 0;
        dstAccess = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        srcQueueIndex = driver->context->queue_info.compute;
        dstQueueIndex = driver->context->queue_info.graphics;
    }
    else if (type == VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE)
    {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        srcAccess = 0;
        dstAccess = VK_ACCESS_SHADER_WRITE_BIT;
        srcQueueIndex = driver->context->queue_info.graphics;
        dstQueueIndex = driver->context->queue_info.compute;
    }
    else
    {
        log_warn("Unsupported barrier type. Memory buffer barrier won't be acquire.");
        return;
    }

    VkBufferMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.size = b->size;
    barrier.buffer = b->buffer;
    barrier.srcQueueFamilyIndex = srcQueueIndex;
    barrier.dstQueueFamilyIndex = dstQueueIndex;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;

    vkCmdPipelineBarrier(
        cmd_buffer->instance,
        srcStage,
        dstStage,
        0,
        0,
        VK_NULL_HANDLE,
        1,
        &barrier,
        0,
        VK_NULL_HANDLE);
}

void vkapi_driver_release_buffer_barrier(
    vkapi_driver_t* driver,
    vkapi_cmdbuffer_t* cmd_buffer,
    buffer_handle_t handle,
    enum BarrierType type)
{
    // No sync required if the graphics and compute queues are the same.
    if (driver->context->queue_info.graphics == driver->context->queue_info.compute)
    {
        return;
    }

    vkapi_buffer_t* b = vkapi_res_cache_get_buffer(driver->res_cache, handle);

    VkPipelineStageFlags srcStage, dstStage;
    VkAccessFlags srcAccess, dstAccess;
    uint32_t srcQueueIndex, dstQueueIndex;

    if (type == VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ)
    {
        srcStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        srcAccess = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        dstAccess = 0;
        srcQueueIndex = driver->context->queue_info.graphics;
        dstQueueIndex = driver->context->queue_info.compute;
    }
    else if (type == VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE)
    {
        srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        srcAccess = VK_ACCESS_SHADER_WRITE_BIT;
        dstAccess = 0;
        srcQueueIndex = driver->context->queue_info.compute;
        dstQueueIndex = driver->context->queue_info.graphics;
    }
    else
    {
        log_warn("Unsupported barrier type. Memory buffer barrier won't be released.");
        return;
    }

    VkBufferMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.size = b->size;
    barrier.buffer = b->buffer;
    barrier.srcQueueFamilyIndex = srcQueueIndex;
    barrier.dstQueueFamilyIndex = dstQueueIndex;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;

    vkCmdPipelineBarrier(
        cmd_buffer->instance,
        srcStage,
        dstStage,
        0,
        0,
        VK_NULL_HANDLE,
        1,
        &barrier,
        0,
        VK_NULL_HANDLE);
}

void vkapi_driver_clear_gpu_buffer(
    vkapi_driver_t* driver, vkapi_cmdbuffer_t* cmd_buffer, buffer_handle_t handle)
{
    vkapi_buffer_t* b = vkapi_res_cache_get_buffer(driver->res_cache, handle);
    vkCmdFillBuffer(cmd_buffer->instance, b->buffer, 0, VK_WHOLE_SIZE, 0);
}

void vkapi_driver_flush_gfx_cmds(vkapi_driver_t* driver)
{
    assert(driver);
    vkapi_commands_flush(driver->context, driver->commands);
}

void vkapi_driver_flush_compute_cmds(vkapi_driver_t* driver)
{
    assert(driver);
    vkapi_commands_flush(driver->context, driver->compute_commands);
}

vkapi_cmdbuffer_t* vkapi_driver_get_compute_cmds(vkapi_driver_t* driver)
{
    assert(driver);
    return vkapi_commands_get_cmdbuffer(driver->context, driver->compute_commands);
}

vkapi_cmdbuffer_t* vkapi_driver_get_gfx_cmds(vkapi_driver_t* driver)
{
    assert(driver);
    return vkapi_commands_get_cmdbuffer(driver->context, driver->commands);
}

void vkapi_driver_gc(vkapi_driver_t* driver)
{
    if (driver->settings.threaded_cmdbuffers)
    {
        for (int i = 0; i < VKAPI_COMMANDS_MAX_THREAD_COUNT; ++i)
        {
            vkapi_pline_cache_gc(driver->thread_draw_info[i].pline_cache, driver->current_frame);
            vkapi_desc_cache_gc(driver->thread_draw_info[i].desc_cache, driver->current_frame);
        }
    }

    vkapi_commands_gc(driver->commands, driver);
    vkapi_pline_cache_gc(driver->pline_cache, driver->current_frame);
    vkapi_desc_cache_gc(driver->desc_cache, driver->current_frame);
    vkapi_res_cache_gc(driver->res_cache, driver);
    vkapi_staging_gc(driver->staging_pool, driver->vma_allocator, driver->current_frame);
    vkapi_fb_cache_gc(driver->framebuffer_cache, driver, driver->current_frame);
}
