/* Copyright (c) 2022 Garry Whitehead
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

#ifndef __BACKEND_CONVERT_TO_VK_H__
#define __BACKEND_CONVERT_TO_VK_H__

#include "enums.h"
#include "volk.h"

VkSamplerAddressMode sampler_addr_mode_to_vk(enum SamplerAddressMode mode);

VkFilter sampler_filter_to_vk(enum SamplerFilter filter);

VkAttachmentLoadOp load_flags_to_vk(enum LoadClearFlags flags);

VkAttachmentStoreOp store_flags_to_vk(enum StoreClearFlags flags);

VkSampleCountFlagBits samples_to_vk(uint32_t count);

VkCompareOp compare_op_to_vk(enum CompareOp op);

VkCullModeFlags cull_mode_to_vk(enum CullMode mode);

VkFrontFace front_face_to_vk(enum FrontFace ff);

VkPolygonMode polygon_mode_to_vk(enum PolygonMode mode);

VkPrimitiveTopology primitive_topology_to_vk(enum PrimitiveTopology topo);

VkBlendFactor blend_factor_to_vk(enum BlendFactor factor);

VkBlendOp blend_op_to_vk(enum BlendOp op);

#endif