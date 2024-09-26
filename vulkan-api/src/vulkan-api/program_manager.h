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

#ifndef __VKAPI_PROGRAM_MANAGER_H__
#define __VKAPI_PROGRAM_MANAGER_H__

#include "common.h"
#include "driver.h"
#include "pipeline.h"
#include "pipeline_cache.h"

#include <utility/arena.h>
#include <utility/hash_set.h>
#include <utility/string.h>

// Forward declarations.
typedef struct Shader shader_t;
typedef struct PipelineLayout vkapi_pl_layout_t;
typedef struct TextureHandle texture_handle_t;
typedef struct ShaderProgram shader_program_t;
typedef struct ProgramManager program_manager_t;
typedef struct ShaderProgramBundle shader_prog_bundle_t;

typedef struct Variant
{
    string_t definition;
    uint value;
} variant_t;

typedef struct RasterState
{
    VkCullModeFlagBits cull_mode;
    VkPolygonMode polygon_mode;
    VkFrontFace front_face;
} raster_state_t;

typedef struct DepthStencilState
{
    struct StencilState
    {
        VkBool32 use_stencil;
        VkStencilOp fail_op;
        VkStencilOp pass_op;
        VkStencilOp depth_fail_op;
        VkStencilOp stencil_fail_op;
        VkCompareOp compare_op;
        uint32_t compare_mask;
        uint32_t write_mask;
        uint32_t reference;
        VkBool32 front_equal_back;
    };

    // Depth state.
    VkBool32 test_enable;
    VkBool32 write_enable;
    VkBool32 stencil_test_enable;
    VkCompareOp compare_op;

    // Only processed if the above is true.
    struct StencilState frontStencil;
    struct StencilState backStencil;
} depth_stencil_state_t;

typedef struct BlendFactorState
{
    VkBool32 blend_enable;
    VkBlendFactor src_colour;
    VkBlendFactor dst_colour;
    VkBlendOp colour;
    VkBlendFactor src_alpha;
    VkBlendFactor dst_alpha;
    VkBlendOp alpha;
} blend_factor_state_t;

typedef struct RenderPrimitive
{
    uint32_t indices_count;
    uint32_t offset;
    uint32_t vertex_count;
    VkPrimitiveTopology topology;
    VkBool32 prim_restart;
    VkIndexType index_buffer_type;
} render_prim_t;

typedef struct DescriptorBindInfo
{
    uint32_t binding;
    VkBuffer buffer;
    uint32_t size;
    VkDescriptorType type;
} desc_bind_info_t;

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wpadded"

typedef struct CachedKey
{
    uint64_t variant_bits;
    uint32_t shader_id;
    uint32_t shader_stage;
    uint32_t topology;
    uint8_t padding[4];
} shader_cache_key_t;

#pragma clang diagnostic pop

/* Shader program functions */

shader_program_t* shader_program_init(arena_t* arena);

void shader_program_add_attr_block(shader_program_t* prog, string_t* block);

/* Shader bundle functions */

shader_prog_bundle_t* shader_bundle_init(arena_t* arena);

/**

 @param bundle
 @param filename
 @param stage
 @param variants
 @param variant_count
 @param arena
 @param scratch_arena
 @return
 */
bool shader_bundle_build_shader(
    shader_prog_bundle_t* bundle,
    const char* filename,
    enum ShaderStage stage,
    variant_t* variants,
    uint32_t variant_count,
    arena_t* arena,
    arena_t* scratch_arena);

bool shader_bundle_parse_mat_shader(
    shader_prog_bundle_t* bundle, const char* shader_path, arena_t* arena, arena_t* scratch_arena);

shader_program_t*
shader_bundle_get_stage_program(shader_prog_bundle_t* bundle, enum ShaderStage stage);

/* Program manager functions */

program_manager_t* program_manager_init(arena_t* arena);

/**
 Check whether the shader exists in the cache based on the key, if not try and compile. Also
 updates the pipeline layout with the descriptor information associated with the shader.
 @note The shader bundle must be first built using @sa shader_bundle_build_shader and if
 using materials, @sa shader_bundle_parse_mat_shader.
 @param manager A pointer to the program manager state.
 @param context A pointer to the Vulkan context.
 @param stage The shader stage which to find or create. Used as the cache key.
 @param topo The Vulkan topology which will be associated with this shader - used as the cache key.
 @param bundle An initialised shader bundle.
 @param variant_bits
 @param arena An arena allocator.
 @return
 */
shader_t* program_manager_find_shader_variant_or_create(
    program_manager_t* manager,
    vkapi_context_t* context,
    enum ShaderStage stage,
    VkPrimitiveTopology topo,
    shader_prog_bundle_t* bundle,
    uint64_t variant_bits,
    arena_t* arena);

#endif
