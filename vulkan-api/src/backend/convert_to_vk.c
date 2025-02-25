
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

#include "convert_to_vk.h"


/*vk::BlendFactor blendFactorToVk(backend::BlendFactor factor)
{
    vk::BlendFactor output;
    switch (factor)
    {
        case backend::BlendFactor::Zero:
            output = vk::BlendFactor::eZero;
            break;
        case backend::BlendFactor::One:
            output = vk::BlendFactor::eOne;
            break;
        case backend::BlendFactor::SrcColour:
            output = vk::BlendFactor::eSrcColor;
            break;
        case backend::BlendFactor::OneMinusSrcColour:
            output = vk::BlendFactor::eOneMinusSrcColor;
            break;
        case backend::BlendFactor::DstColour:
            output = vk::BlendFactor::eDstColor;
            break;
        case backend::BlendFactor::OneMinusDstColour:
            output = vk::BlendFactor::eOneMinusDstColor;
            break;
        case backend::BlendFactor::SrcAlpha:
            output = vk::BlendFactor::eSrcAlpha;
            break;
        case backend::BlendFactor::OneMinusSrcAlpha:
            output = vk::BlendFactor::eOneMinusSrcAlpha;
            break;
        case backend::BlendFactor::DstAlpha:
            output = vk::BlendFactor::eDstAlpha;
            break;
        case backend::BlendFactor::OneMinusDstAlpha:
            output = vk::BlendFactor::eOneMinusDstAlpha;
            break;
        case backend::BlendFactor::ConstantColour:
            output = vk::BlendFactor::eConstantColor;
            break;
        case backend::BlendFactor::OneMinusConstantColour:
            output = vk::BlendFactor::eOneMinusConstantColor;
            break;
        case backend::BlendFactor::ConstantAlpha:
            output = vk::BlendFactor::eConstantAlpha;
            break;
        case backend::BlendFactor::OneMinusConstantAlpha:
            output = vk::BlendFactor::eOneMinusConstantAlpha;
            break;
        case backend::BlendFactor::SrcAlphaSaturate:
            output = vk::BlendFactor::eSrcAlphaSaturate;
            break;
        default:
            SPDLOG_WARN("Unrecognised blend factor when converting to Vk.");
            break;
    }
    return output;
}

vk::BlendOp blendOpToVk(backend::BlendOp op)
{
    vk::BlendOp output;
    switch (op)
    {
        case backend::BlendOp::Subtract:
            output = vk::BlendOp::eSubtract;
            break;
        case backend::BlendOp::ReverseSubtract:
            output = vk::BlendOp::eReverseSubtract;
            break;
        case backend::BlendOp::Add:
            output = vk::BlendOp::eAdd;
            break;
        case backend::BlendOp::Min:
            output = vk::BlendOp::eMin;
            break;
        case backend::BlendOp::Max:
            output = vk::BlendOp::eMax;
            break;
        default:
            SPDLOG_WARN("Unrecognised blend op when converting to Vk.");
            break;
    }
    return output;
}*/

VkSamplerAddressMode sampler_addr_mode_to_vk(enum SamplerAddressMode mode)
{
    VkSamplerAddressMode output;
    switch (mode)
    {
        case RPE_SAMPLER_ADDR_MODE_REPEAT:
            output = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case RPE_SAMPLER_ADDR_MODE_MIRRORED_REPEAT:
            output = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        case RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE:
            output = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case RPE_SAMPLER_ADDR_MODE_CLAMP_TO_BORDER:
            output = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            break;
        case RPE_SAMPLER_ADDR_MODE_MIRROR_CLAMP_TO_EDGE:
            output = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            break;
    }
    return output;
}

VkFilter sampler_filter_to_vk(enum SamplerFilter filter)
{
    VkFilter output;
    switch (filter)
    {
        case RPE_SAMPLER_FILTER_LINEAR:
            output = VK_FILTER_LINEAR;
            break;
        case RPE_SAMPLER_FILTER_NEAREST:
            output = VK_FILTER_NEAREST;
            break;
        case RPE_SAMPLER_FILTER_CUBIC:
            output = VK_FILTER_CUBIC_IMG;
            break;
    }
    return output;
}

VkCullModeFlags cull_mode_to_vk(enum CullMode mode)
{
    VkCullModeFlags output;
    switch (mode)
    {
        case RPE_CULL_MODE_BACK:
            output = VK_CULL_MODE_BACK_BIT;
            break;
        case RPE_CULL_MODE_FRONT:
            output = VK_CULL_MODE_FRONT_BIT;
            break;
        case RPE_CULL_MODE_NONE:
            output = VK_CULL_MODE_NONE;
            break;
    }
    return output;
}

VkFrontFace front_face_to_vk(enum FrontFace ff)
{
    VkFrontFace out;
    switch (ff)
    {
        case RPE_FRONT_FACE_CLOCKWISE:
            out = VK_FRONT_FACE_CLOCKWISE;
            break;
        case RPE_FRONT_FACE_COUNTER_CLOCKWISE:
            out = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            break;
    }
    return out;
}

VkPolygonMode polygon_mode_to_vk(enum PolygonMode mode)
{
    VkPolygonMode out;
    switch (mode)
    {
        case RPE_POLYGON_MODE_FILL:
            out = VK_POLYGON_MODE_FILL;
            break;
        case RPE_POLYGON_MODE_LINE:
            out = VK_POLYGON_MODE_LINE;
            break;
        case RPE_POLYGON_MODE_POINT:
            out = VK_POLYGON_MODE_POINT;
            break;
    }
    return out;
}

VkCompareOp compare_op_to_vk(enum CompareOp op)
{
    VkCompareOp output;
    switch (op)
    {
        case RPE_COMPARE_OP_NEVER:
            output = VK_COMPARE_OP_NEVER;
            break;
        case RPE_COMPARE_OP_ALWAYS:
            output = VK_COMPARE_OP_ALWAYS;
            break;
        case RPE_COMPARE_OP_EQUAL:
            output = VK_COMPARE_OP_EQUAL;
            break;
        case RPE_COMPARE_OP_GREATER:
            output = VK_COMPARE_OP_GREATER;
            break;
        case RPE_COMPARE_OP_GREATER_OR_EQUAL:
            output = VK_COMPARE_OP_GREATER_OR_EQUAL;
            break;
        case RPE_COMPARE_OP_LESS:
            output = VK_COMPARE_OP_LESS;
            break;
        case RPE_COMPARE_OP_LESS_OR_EQUAL:
            output = VK_COMPARE_OP_LESS_OR_EQUAL;
            break;
        case RPE_COMPARE_OP_NOT_EQUAL:
            output = VK_COMPARE_OP_NOT_EQUAL;
            break;
    }
    return output;
}

VkPrimitiveTopology primitive_topology_to_vk(enum PrimitiveTopology topo)
{
    VkPrimitiveTopology output;
    switch (topo)
    {
        case RPE_TOPOLOGY_POINT_LIST:
            output = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            break;
        case RPE_TOPOLOGY_LINE_LIST:
            output = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            break;
        case RPE_TOPOLOGY_LINE_STRIP:
            output = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            break;
        case RPE_TOPOLOGY_TRIANGLE_LIST:
            output = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            break;
        case RPE_TOPOLOGY_TRIANGLE_STRIP:
            output = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            break;
        case RPE_TOPOLOGY_TRIANGLE_FAN:
            output = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            break;
        case RPE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
            output = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
            break;
        case RPE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
            output = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
            break;
        case RPE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
            output = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
            break;
        case RPE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
            output = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
            break;
        case RPE_TOPOLOGY_PATCH_LIST:
            output = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
            break;
    }
    return output;
}

/*vk::IndexType indexBufferTypeToVk(backend::IndexBufferType type)
{
    vk::IndexType output;
    switch (type)
    {
        case IndexBufferType::Uint32:
            output = vk::IndexType::eUint32;
            break;
        case IndexBufferType::Uint16:
            output = vk::IndexType::eUint16;
            break;
        default:
            SPDLOG_WARN("Unsupported index buffer type.");
            break;
    }
    return output;
}

vk::Format textureFormatToVk(backend::TextureFormat type)
{
    vk::Format output = vk::Format::eUndefined;

    switch (type)
    {
        case backend::TextureFormat::R8:
            output = vk::Format::eR8Unorm;
            break;
        case backend::TextureFormat::R16F:
            output = vk::Format::eR16Sfloat;
            break;
        case backend::TextureFormat::R32F:
            output = vk::Format::eR32Sfloat;
            break;
        case backend::TextureFormat::R32U:
            output = vk::Format::eR32Uint;
            break;
        case backend::TextureFormat::RG8:
            output = vk::Format::eR8G8Unorm;
            break;
        case backend::TextureFormat::RG16F:
            output = vk::Format::eR16G16Sfloat;
            break;
        case backend::TextureFormat::RG32F:
            output = vk::Format::eR32G32Sfloat;
            break;
        case backend::TextureFormat::RGB8:
            output = vk::Format::eR8G8B8Unorm;
            break;
        case backend::TextureFormat::RGB16F:
            output = vk::Format::eR16G16B16Sfloat;
            break;
        case backend::TextureFormat::RGB32F:
            output = vk::Format::eR32G32B32Sfloat;
            break;
        case backend::TextureFormat::RGBA8:
            output = vk::Format::eR8G8B8A8Unorm;
            break;
        case backend::TextureFormat::RGBA16F:
            output = vk::Format::eR16G16B16A16Sfloat;
            break;
        case backend::TextureFormat::RGBA32F:
            output = vk::Format::eR32G32B32A32Sfloat;
            break;
    }
    return output;
}

vk::ImageUsageFlags imageUsageToVk(uint32_t usageFlags)
{
    vk::ImageUsageFlags output;
    if (usageFlags & ImageUsage::Sampled)
    {
        output |= vk::ImageUsageFlagBits::eSampled;
    }
    if (usageFlags & ImageUsage::Storage)
    {
        output |= vk::ImageUsageFlagBits::eStorage;
    }
    if (usageFlags & ImageUsage::ColourAttach)
    {
        output |= vk::ImageUsageFlagBits::eColorAttachment;
    }
    if (usageFlags & ImageUsage::DepthAttach)
    {
        output |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
    }
    if (usageFlags & ImageUsage::InputAttach)
    {
        output |= vk::ImageUsageFlagBits::eInputAttachment;
    }
    if (usageFlags & ImageUsage::Src)
    {
        output |= vk::ImageUsageFlagBits::eTransferSrc;
    }
    if (usageFlags & ImageUsage::Dst)
    {
        output |= vk::ImageUsageFlagBits::eTransferDst;
    }
    return output;
}*/

VkAttachmentLoadOp load_flags_to_vk(enum LoadClearFlags flags)
{
    VkAttachmentLoadOp result;
    switch (flags)
    {
        case RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR:
            result = VK_ATTACHMENT_LOAD_OP_CLEAR;
            break;
        case RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_DONTCARE:
            result = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            break;
        case RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_LOAD:
            result = VK_ATTACHMENT_LOAD_OP_LOAD;
            break;
    }
    return result;
}

VkAttachmentStoreOp store_flags_to_vk(enum StoreClearFlags flags)
{
    VkAttachmentStoreOp result;
    switch (flags)
    {
        case RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE:
            result = VK_ATTACHMENT_STORE_OP_STORE;
            break;
        case RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_DONTCARE:
            result = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            break;
    }
    return result;
}

VkSampleCountFlagBits samples_to_vk(uint32_t count)
{
    VkSampleCountFlagBits result = VK_SAMPLE_COUNT_1_BIT;
    switch (count)
    {
        case 1:
            result = VK_SAMPLE_COUNT_1_BIT;
            break;
        case 2:
            result = VK_SAMPLE_COUNT_2_BIT;
            break;
        case 4:
            result = VK_SAMPLE_COUNT_4_BIT;
            break;
        case 8:
            result = VK_SAMPLE_COUNT_8_BIT;
            break;
        case 16:
            result = VK_SAMPLE_COUNT_16_BIT;
            break;
        case 32:
            result = VK_SAMPLE_COUNT_32_BIT;
            break;
        case 64:
            result = VK_SAMPLE_COUNT_64_BIT;
            break;
        default:
            log_warn("Unsupported sample count. Set to one.");
            break;
    }
    return result;
}
