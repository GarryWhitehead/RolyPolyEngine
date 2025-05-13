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

#include "renderpass.h"

#include "backend/convert_to_vk.h"
#include "driver.h"
#include "utility.h"

#include <string.h>
#include <utility/arena.h>

vkapi_render_target_t vkapi_render_target_init()
{
    vkapi_render_target_t rt = {0};
    for (int i = 0; i < VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT; ++i)
    {
        vkapi_invalidate_tex_handle(&rt.colours[i].handle);
    }
    vkapi_invalidate_tex_handle(&rt.depth.handle);
    rt.clear_colour.a = 1.0f;
    return rt;
}

vkapi_rpass_t vkapi_rpass_init(arena_t* arena)
{
    vkapi_rpass_t rpass = {0};
    MAKE_DYN_ARRAY(VkAttachmentDescription, arena, 10, &rpass.attach_descriptors);
    MAKE_DYN_ARRAY(VkAttachmentReference, arena, 10, &rpass.colour_attach_refs);
    return rpass;
}

vkapi_attach_handle_t vkapi_rpass_add_attach(vkapi_rpass_t* rp, struct VkApiAttachment* attach)
{
    VkAttachmentDescription ad = {0};
    ad.format = attach->format;
    ad.initialLayout = attach->initial_layout;
    ad.finalLayout = attach->final_layout;

    // samples
    ad.samples = samples_to_vk(attach->sample_count);

    // clear flags
    ad.loadOp = load_flags_to_vk(attach->load_op); // pre image state
    ad.storeOp = store_flags_to_vk(attach->store_op); // post image state
    ad.stencilLoadOp = load_flags_to_vk(attach->stencil_load_op); // pre stencil state
    ad.stencilStoreOp = store_flags_to_vk(attach->stencil_store_op); // post stencil state

    vkapi_attach_handle_t handle = {.id = rp->attach_descriptors.size};
    DYN_ARRAY_APPEND(&rp->attach_descriptors, &ad);
    return handle;
}

void vkapi_rpass_create(vkapi_rpass_t* rp, vkapi_driver_t* driver, uint32_t multi_view_count)
{
    // create the attachment references
    bool surfacePass = false;
    VkAttachmentReference refs[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT] = {0};

    for (size_t count = 0; count < rp->attach_descriptors.size; ++count)
    {
        VkAttachmentDescription* desc =
            DYN_ARRAY_GET_PTR(VkAttachmentDescription, &rp->attach_descriptors, count);
        assert(desc);
        if (desc->finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            surfacePass = true;
        }

        VkImageLayout image_layout =
            vkapi_util_is_stencil(desc->format) || vkapi_util_is_depth(desc->format)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        refs[count].attachment = count;
        refs[count].layout = image_layout;
        if (vkapi_util_is_depth(desc->format) || vkapi_util_is_stencil(desc->format))
        {
            rp->depth_attach_desc = &refs[count];
        }
        else
        {
            DYN_ARRAY_APPEND(&rp->colour_attach_refs, &refs[count]);
        }
    }

    // add the dependdencies
    memset(rp->subpass_dep, 0, sizeof(VkSubpassDependency) * 2);
    rp->subpass_dep[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    rp->subpass_dep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    rp->subpass_dep[0].dstSubpass = 0;
    rp->subpass_dep[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    rp->subpass_dep[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    rp->subpass_dep[1].srcSubpass = rp->subpass_dep[0].dstSubpass;
    rp->subpass_dep[1].dstSubpass = rp->subpass_dep[0].srcSubpass;
    rp->subpass_dep[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    if (!rp->colour_attach_refs.size && rp->depth_attach_desc)
    {
        rp->subpass_dep[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        rp->subpass_dep[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        rp->subpass_dep[1].dstStageMask = rp->subpass_dep[0].srcStageMask;
        rp->subpass_dep[1].dstAccessMask = rp->subpass_dep[0].srcAccessMask;
        rp->subpass_dep[1].srcStageMask = rp->subpass_dep[0].dstStageMask;
        rp->subpass_dep[1].srcAccessMask = rp->subpass_dep[0].dstAccessMask;
    }
    else if (surfacePass)
    {
        rp->subpass_dep[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        rp->subpass_dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        rp->subpass_dep[0].srcAccessMask = 0;
        rp->subpass_dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        rp->subpass_dep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        rp->subpass_dep[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else
    {
        // Colour pass.
        rp->subpass_dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        rp->subpass_dep[0].dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        rp->subpass_dep[1].dstStageMask = rp->subpass_dep[0].srcStageMask;
        rp->subpass_dep[1].dstAccessMask = rp->subpass_dep[0].srcAccessMask;
        rp->subpass_dep[1].srcStageMask = rp->subpass_dep[0].dstStageMask;
        rp->subpass_dep[1].srcAccessMask = rp->subpass_dep[0].dstAccessMask;
    }

    VkSubpassDescription sp = {0};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = rp->colour_attach_refs.size;
    sp.pColorAttachments = rp->colour_attach_refs.data;
    sp.pDepthStencilAttachment = rp->depth_attach_desc ? rp->depth_attach_desc : VK_NULL_HANDLE;

    VkRenderPassCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = rp->attach_descriptors.size;
    ci.pAttachments = rp->attach_descriptors.data;
    ci.subpassCount = 1;
    ci.pSubpasses = &sp;
    ci.dependencyCount = 2;
    ci.pDependencies = rp->subpass_dep;

    VkRenderPassMultiviewCreateInfo mv_ci = {};
    mv_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
    uint32_t* view_masks =
        ARENA_MAKE_ARRAY(&driver->_scratch_arena, uint32_t, rp->attach_descriptors.size, 0);
    uint32_t* correlation_masks =
        ARENA_MAKE_ARRAY(&driver->_scratch_arena, uint32_t, rp->attach_descriptors.size, 0);
    if (multi_view_count > 0)
    {
        uint32_t mask = 0;
        for (uint32_t i = 0; i < multi_view_count; ++i)
        {
            mask |= 1 << i;
        }

        for (size_t i = 0; i < rp->attach_descriptors.size; ++i)
        {
            view_masks[i] = mask;
            correlation_masks[i] = view_masks[i];
        }
        mv_ci.correlationMaskCount = rp->attach_descriptors.size;
        mv_ci.pCorrelationMasks = correlation_masks;
        mv_ci.subpassCount = 1;
        mv_ci.pViewMasks = view_masks;
        ci.pNext = &mv_ci;
    }
    VK_CHECK_RESULT(vkCreateRenderPass(driver->context->device, &ci, VK_NULL_HANDLE, &rp->instance))
    arena_reset(&driver->_scratch_arena);
}

vkapi_fbo_t vkapi_fbo_init()
{
    vkapi_fbo_t i;
    memset(&i, 0, sizeof(vkapi_fbo_t));
    return i;
}

void vkapi_fbo_create(
    vkapi_fbo_t* fbo,
    vkapi_driver_t* driver,
    VkRenderPass rp,
    VkImageView* image_views,
    uint32_t count,
    uint32_t width,
    uint32_t height,
    uint8_t layers)
{
    assert(width > 0);
    assert(height > 0);
    assert(image_views > 0);

    fbo->width = width;
    fbo->height = height;

    VkFramebufferCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.width = width;
    ci.height = height;
    ci.layers = layers;
    ci.attachmentCount = count;
    ci.pAttachments = image_views;
    ci.renderPass = rp;

    VK_CHECK_RESULT(
        vkCreateFramebuffer(driver->context->device, &ci, VK_NULL_HANDLE, &fbo->instance));
}

uint32_t vkapi_rpass_get_attach_count(vkapi_rpass_t* rpass)
{
    assert(rpass);
    return rpass->depth_attach_desc ? rpass->attach_descriptors.size - 1
                                    : rpass->attach_descriptors.size;
}
