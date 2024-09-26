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

#include "program_manager.h"

#include "backend/enums.h"
#include "driver.h"
#include "pipeline_cache.h"
#include "shader.h"
#include "shader_util.h"

#include <assert.h>
#include <string.h>
#include <utility/filesystem.h>
#include <utility/hash.h>

#define LINE_BUFFER_SIZE 100

typedef struct ShaderProgram
{
    /// used for creating the glsl string representation used for compilation.
    /// the main() code section.
    string_t main_stage_block;
    /// The attribute descriptors for the main code block.
    string_t attr_desc_block;
    /// Additional code which is specific to the material.
    string_t mat_shader_block;
    /// Additional attributes (ubos, ssbos, samplers).
    arena_dyn_array_t attr_blocks;
    /// Keep a track of include statement within the shader.
    arena_dyn_array_t includes;

    arena_t* arena;
    /// Shaders used by this program. Note: these are not owned by the program
    /// but by the cached container in the program manager.
    shader_t* shader;

} shader_program_t;

typedef struct ShaderProgramBundle
{
    // The id of this program bundle
    size_t shader_id;

    shader_program_t* programs[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];

    // Information about the pipeline layout associated with this shader
    // program. This is derived from shader reflection.
    vkapi_pl_layout_t* pipeline_layout;

    // Note: this is initialised after a call to update_desc_layouts().
    struct PushBlockBindParams push_blocks[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];

    struct ImageSamplerParams
    {
        texture_handle_t* handle;
        VkSampler sampler;
    };

    // Index into the resource cache for the texture for each attachment.
    struct ImageSamplerParams image_samplers[VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT];

    texture_handle_t* storage_images[VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT];

    // We keep a record of descriptors here and their binding info for
    // use at the pipeline binding draw stage.
    arena_dyn_array_t desc_bind_info;

    render_prim_t render_prim;

    VkRect2D scissor;
    VkViewport viewport;

    size_t tesse_vert_count;

} shader_prog_bundle_t;

typedef struct ProgramManager
{
    /// Fully compiled, complete shader programs.
    arena_dyn_array_t program_bundles;

    // This is where individual shaders are cached until required to assemble
    // into a complete shader program.
    hash_set_t shader_cache;

} program_manager_t;

shader_program_t* shader_program_init(arena_t* arena)
{
    assert(arena);
    shader_program_t* out = ARENA_MAKE_STRUCT(arena, shader_program_t, ARENA_ZERO_MEMORY);
    MAKE_DYN_ARRAY(string_t, arena, 30, &out->attr_blocks);
    MAKE_DYN_ARRAY(string_t, arena, 15, &out->includes);
    out->arena = arena;
    return out;
}

void shader_program_add_attr_block(shader_program_t* prog, string_t* block)
{
    assert(prog);
    assert(block);
    DYN_ARRAY_APPEND(&prog->attr_blocks, block);
}

string_t shader_program_build(shader_program_t* prog, arena_t* arena)
{
    assert(prog);

    string_t shader_block = string_init("#version 460\n\n", arena);

    // Add any include file pre-processor commands - these are added at the top of the
    // shader file as the contents may be required by other members
    // declared within the rest of the code block.
    for (uint32_t i = 0; i < prog->includes.size; ++i)
    {
        string_t include_str = DYN_ARRAY_GET(string_t, &prog->includes, i);
        shader_util_append_include_file(&shader_block, &include_str, arena);
    }

    // Append any additional blocks to the shader code. This consists of Ubos,
    // samplers, etc.
    for (uint32_t i = 0; i < prog->attr_blocks.size; ++i)
    {
        string_t attr_block_str = DYN_ARRAY_GET(string_t, &prog->attr_blocks, i);
        shader_block = string_append(&shader_block, attr_block_str.data, arena);
    }

    // Add the main attributes.
    shader_block = string_append(&shader_block, prog->attr_desc_block.data, arena);

    // add the material shader code if defined.
    if (prog->mat_shader_block.data)
    {
        shader_block = string_append(&shader_block, prog->mat_shader_block.data, arena);
    }

    // Append the main shader code block.
    return string_append(&shader_block, prog->main_stage_block.data, prog->arena);
}

bool shader_program_parse_mat_shader_block(
    shader_program_t* prog, FILE* fp, char* buffer, arena_t* arena)
{
    assert(fp);
    assert(buffer);

    if (!prog)
    {
        log_error("Shader program hasn't been created for this stage. The function "
                  "shader_bundle_build_shader() must be called before calling this function.");
        return false;
    }

    while (fgets(buffer, LINE_BUFFER_SIZE, fp))
    {
        string_t line;
        line.data = buffer;
        line.len = strlen(buffer);

        // If we have found the next shader stage denoter, then break out.
        if (string_contains(&line, "[[end]]"))
        {
            return true;
        }
        if (string_contains(&line, "#include \""))
        {
            shader_util_append_include_file(&prog->mat_shader_block, &line, arena);
            continue;
        }
        prog->mat_shader_block = string_append(&prog->mat_shader_block, buffer, arena);
    }

    log_error("Material shader block is missing [[end]] designator.");
    return false;
}

bool shader_prog_parse_shader(
    shader_program_t* prog,
    const char* shader_path,
    variant_t* variants,
    uint32_t variant_count,
    arena_t* arena,
    arena_t* scratch_arena)
{
    assert(prog);
    assert(shader_path);

    string_t dir_path = string_init(RPE_SHADER_DIRECTORY, scratch_arena);
    string_t abs_path = string_append3(&dir_path, "/", shader_path, scratch_arena);
    FILE* fp = fopen(abs_path.data, "r");
    if (!fp)
    {
        log_error("Error whilst loading shader: %s", shader_path);
        return false;
    }

    char line[LINE_BUFFER_SIZE];
    while (fgets(line, LINE_BUFFER_SIZE, fp))
    {
        string_t s;
        s.data = line;
        s.len = strlen(line);

        if (string_contains(&s, "#include \""))
        {
            // skip adding this to the code block. Include statements will
            // instead be added to the top of the completed text block.
            // Get the #include path and store for later.
            uint32_t count;
            string_t** split = string_split(&s, ' ', &count, arena);
            if (count <= 1)
            {
                log_error("Invalid #include syntax: %s", line);
                return false;
            }
            DYN_ARRAY_APPEND(&prog->includes, split[1]);
            continue;
        }
        if (string_contains(&s, "void main()"))
        {
            prog->main_stage_block = string_init(s.data, arena);
            // Keep going until the end of the file.
            while (fgets(line, LINE_BUFFER_SIZE, fp))
            {
                prog->main_stage_block =
                    string_append(&prog->main_stage_block, line, arena);
            }
            break;
        }
        prog->attr_desc_block = string_append(&prog->attr_desc_block, line, arena);
    }

    if (!prog->main_stage_block.len)
    {
        log_error("Shader code block contains no main() source.");
        return false;
    }

    prog->attr_desc_block = shader_program_process_preprocessor(
        &prog->attr_desc_block, variants, variant_count, arena, scratch_arena);
    if (!prog->attr_desc_block.data)
    {
        log_error("Eror whilst parsing shader: %s", shader_path);
        return false;
    }
    return true;
}

bool shader_bundle_parse_mat_shader(
    shader_prog_bundle_t* bundle, const char* shader_path, arena_t* arena, arena_t* scratch_arena)
{
    string_t abs_path = string_init(RPE_SHADER_DIRECTORY, scratch_arena);
    string_t path = string_append3(&abs_path, "/materials/", shader_path, scratch_arena);

    FILE* fp = fopen(path.data, "r");
    if (!fp)
    {
        log_error("Error whilst loading material shader: %s", shader_path);
        return false;
    }

    // we use the material shader as the hash for the key
    bundle->shader_id = murmur_hash3((void*)shader_path, strlen(shader_path));

    char buffer[LINE_BUFFER_SIZE];
    while (fgets(buffer, LINE_BUFFER_SIZE, fp))
    {
        string_t line;
        line.data = buffer;
        line.len = strlen(buffer);

        if (string_contains(&line, "[[vertex]]"))
        {
            if (!shader_program_parse_mat_shader_block(
                    bundle->programs[RPE_BACKEND_SHADER_STAGE_VERTEX],
                    fp,
                    buffer,
                    arena))
            {
                log_error("Error whilst parsing material vertex block for shader: %s", shader_path);
                return false;
            }
        }
        else if (string_contains(&line, "[[fragment]]"))
        {
            if (!shader_program_parse_mat_shader_block(
                    bundle->programs[RPE_BACKEND_SHADER_STAGE_FRAGMENT],
                    fp,
                    buffer,
                    arena))
            {
                log_error(
                    "Error whilst parsing material fragment block for shader: %s", shader_path);
                return false;
            }
        }
        else if (string_contains(&line, "[[tesse-eval]]"))
        {
            if (!shader_program_parse_mat_shader_block(
                    bundle->programs[RPE_BACKEND_SHADER_STAGE_TESSE_EVAL],
                    fp,
                    buffer,
                    arena))
            {
                log_error(
                    "Error whilst parsing material tesse-eval block for shader: %s", shader_path);
                return false;
            }
        }
        else if (string_contains(&line, "[[tesse-control]]"))
        {
            if (!shader_program_parse_mat_shader_block(
                    bundle->programs[RPE_BACKEND_SHADER_STAGE_TESSE_CON],
                    fp,
                    buffer,
                    arena))
            {
                log_error(
                    "Error whilst parsing material tesse-con block for shader: %s", shader_path);
                return false;
            }
        }
    }

    arena_reset(scratch_arena);
    fclose(fp);
    return true;
}

shader_prog_bundle_t* shader_bundle_init(arena_t* arena)
{
    shader_prog_bundle_t* out = ARENA_MAKE_STRUCT(arena, shader_prog_bundle_t, ARENA_ZERO_MEMORY);
    out->pipeline_layout = vkapi_pl_layout_init(arena);
    return out;
}

bool shader_bundle_build_shader(
    shader_prog_bundle_t* bundle,
    const char* filename,
    enum ShaderStage stage,
    variant_t* variants,
    uint32_t variant_count,
    arena_t* arena,
    arena_t* scratch_arena)
{
    // Prefer the material shader filename as the hash key. If this isn't set,
    // use the main shader filename.
    bundle->shader_id =
        !bundle->shader_id ? murmur_hash3((void*)filename, strlen(filename)) : bundle->shader_id;

    bundle->programs[stage] = shader_program_init(arena);
    return shader_prog_parse_shader(
        bundle->programs[stage], filename, variants, variant_count, arena, scratch_arena);
}

void shader_bundle_get_ss_create_info(
    shader_prog_bundle_t* bundle, VkPipelineShaderStageCreateInfo* out, uint32_t* count)
{
    assert(out);
    assert(count);

    *count = 0;
    for (size_t i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        if (bundle->programs[i]->shader)
        {
            out[i] = shader_get_create_info(bundle->programs[i]->shader);
        }
    }
}

void shader_bundle_add_desc_binding(
    shader_prog_bundle_t* bundle,
    uint32_t size,
    uint32_t binding,
    VkBuffer buffer,
    VkDescriptorType type)
{
    assert(buffer);
    assert(size > 0);
    desc_bind_info_t info = {.binding = binding, .buffer = buffer, .size = size, .type = type};
    DYN_ARRAY_APPEND(&bundle->desc_bind_info, &info);
}

void shader_bundle_set_image_sampler(
    shader_prog_bundle_t* bundle, texture_handle_t* handle, uint8_t binding, VkSampler sampler)
{
    assert(handle);
    assert(binding < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT && "Binding of is out of bounds.");
    bundle->image_samplers[binding].handle = handle;
    bundle->image_samplers[binding].sampler = sampler;
}

void shader_bundle_set_storage_image(
    shader_prog_bundle_t* bundle, texture_handle_t* handle, uint8_t binding)
{
    assert(handle);
    assert(binding < VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT);
    bundle->storage_images[binding] = handle;
}

void shader_bundle_set_push_block_data(
    shader_prog_bundle_t* bundle, enum ShaderStage stage, void* data)
{
    assert(data);
    bundle->push_blocks[stage].data = data;
}

void shader_bundle_add_render_primitive(
    shader_prog_bundle_t* bundle,
    VkPrimitiveTopology topo,
    VkIndexType index_buffer_type,
    uint32_t indices_count,
    uint32_t indices_offset,
    VkBool32 prim_restart)
{
    assert(bundle);
    bundle->render_prim.prim_restart = prim_restart;
    bundle->render_prim.topology = topo;
    bundle->render_prim.index_buffer_type = index_buffer_type;
    bundle->render_prim.indices_count = indices_count;
    bundle->render_prim.offset = indices_offset;
}

void shader_bundle_set_scissor(
    shader_prog_bundle_t* bundle,
    uint32_t width,
    uint32_t height,
    uint32_t x_offset,
    uint32_t y_offset)
{
    assert(bundle);
    VkRect2D rect = {
        .offset = {.x = (int32_t)x_offset, .y = (int32_t)y_offset},
        .extent = {.width = width, .height = height}};
    bundle->scissor = rect;
}

void shader_bundle_set_viewport(
    shader_prog_bundle_t* bundle, uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    assert(bundle);
    assert(width > 0);
    assert(height > 0);

    VkViewport vp = {
        .width = (float)width,
        .height = (float)height,
        .x = 0,
        .y = 0,
        .minDepth = minDepth,
        .maxDepth = maxDepth,
    };
    bundle->viewport = vp;
}

void shader_bundle_add_texture_sampler(
    shader_prog_bundle_t* bundle, VkSampler sampler, uint32_t binding)
{
    assert(bundle);
    assert(binding < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT);
    bundle->image_samplers[binding].sampler = sampler;
}

void shader_bundle_create_push_block(
    shader_prog_bundle_t* bundle, size_t size, enum ShaderStage stage)
{
    assert(bundle);
    assert(size > 0);
    bundle->push_blocks[stage].stage = shader_vk_stage_flag(stage);
    bundle->push_blocks[stage].size = size;
}

void shader_bundle_clear(shader_prog_bundle_t* bundle)
{
    dyn_array_clear(&bundle->desc_bind_info);
    vkapi_pl_clear_descs(bundle->pipeline_layout);

    for (size_t i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        if (bundle->programs[i]->shader)
        {
            // shader_program_clear_attr(bundle->programs[i]);
        }
    }
}

shader_program_t* shader_bundle_get_stage_program(shader_prog_bundle_t* bundle, enum ShaderStage stage)
{
    assert(bundle);
    return bundle->programs[stage];
}

program_manager_t* program_manager_init(arena_t* arena)
{
    program_manager_t* out = ARENA_MAKE_STRUCT(arena, program_manager_t, ARENA_ZERO_MEMORY);
    MAKE_DYN_ARRAY(shader_prog_bundle_t, arena, 20, &out->program_bundles);
    out->shader_cache =
        hash_set_create(arena, murmur_hash3, sizeof(shader_cache_key_t), sizeof_shader_t);
    return out;
}

shader_t*
program_manager_find_cached_shader_variant(program_manager_t* manager, shader_cache_key_t* key)
{
    return HASH_SET_GET(&manager->shader_cache, key);
}

shader_t* program_manager_compile_shader(
    program_manager_t* manager,
    vkapi_context_t* context,
    const char* shader_code,
    enum ShaderStage stage,
    shader_cache_key_t* key,
    arena_t* arena)
{
    shader_t* shader = shader_init(stage, arena);
    if (!shader_compile(shader, context, shader_code, "", arena))
    {
        return NULL;
    }
    // cache for later use
    shader_t* out = hash_set_insert(&manager->shader_cache, &key, shader);
    assert(out);
    return out;
}

shader_prog_bundle_t*
program_manager_create_program_bundle(program_manager_t* manager, arena_t* arena)
{
    assert(manager);
    shader_prog_bundle_t* b = shader_bundle_init(arena);
    shader_prog_bundle_t* out = DYN_ARRAY_APPEND(&manager->program_bundles, b);
    assert(out);
    return out;
}

shader_t* program_manager_find_shader_variant_or_create(
    program_manager_t* manager,
    vkapi_context_t* context,
    enum ShaderStage stage,
    VkPrimitiveTopology topo,
    shader_prog_bundle_t* bundle,
    uint64_t variant_bits,
    arena_t* arena)
{
    // Check whether the required variant shader is in the cache and use this if so.
    shader_cache_key_t key = {};
    key.shader_id = bundle->shader_id;
    key.variant_bits = variant_bits;
    key.topology = (uint32_t)topo;
    key.shader_stage = (uint32_t)stage;

    shader_t* shader = program_manager_find_cached_shader_variant(manager, &key);
    if (!shader)
    {
        shader_program_t* p = bundle->programs[stage];
        string_t code_block = shader_program_build(p, arena);
        shader =
            program_manager_compile_shader(manager, context, code_block.data, stage, &key, arena);
        if (!shader)
        {
            return NULL;
        }
    }

    // Update the pipeline layout.
    shader_binding_t* sb = shader_get_resource_binding(shader);
    for (uint32_t i = 0; i < sb->desc_layout_count; ++i)
    {
        shader_desc_layout_t* l = &sb->desc_layouts[i];
        vkapi_pl_layout_add_desc_layout(
            bundle->pipeline_layout, l->set, l->binding, l->type, l->stage, arena);
    }

    if (sb->push_block_size > 0)
    {
        // Push constant details for the pipeline layout and shder bundle.
        vkapi_pl_layout_add_push_const(bundle->pipeline_layout, stage, sb->push_block_size);
        shader_bundle_create_push_block(bundle, sb->push_block_size, stage);
    }

    return shader;
}
