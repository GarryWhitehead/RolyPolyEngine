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

#ifndef __RPE_COMMANDS_H__
#define __RPE_COMMANDS_H__

#include "render_queue.h"

#include <backend/enums.h>
#include <stddef.h>
#include <stdint.h>
#include <vulkan-api/resource_cache.h>

// Forward declarations
typedef struct Arena arena_t;
typedef struct VkApiDriver vkapi_driver_t;
typedef struct CommandPacket rpe_cmd_packet_t;
typedef struct ShaderProgramBundle shader_prog_bundle_t;

typedef void (*DispatchFunction)(vkapi_driver_t*, void*);

typedef struct CommandBucket
{
    rpe_cmd_packet_t** packets;
    uint32_t curr_index;
} rpe_cmd_bucket_t;

typedef struct CommandPacket
{
    void* cmds;
    rpe_cmd_packet_t* next;
    DispatchFunction dispatch_func;
    size_t data_size;
    uint8_t data[];
} rpe_cmd_packet_t;

typedef struct DrawCommand
{
    uint32_t vertex_count;
    uint32_t start_vertex;
} rpe_commands_draw_t;

typedef struct DrawIndexCommand
{
    uint32_t index_count;
    int32_t vertex_offset;
    int32_t index_offset;
} rpe_commands_draw_index_t;

typedef struct DrawIndirectIndexCommand
{
    buffer_handle_t cmd_handle;
    buffer_handle_t count_handle;
    uint32_t offset;
    uint32_t draw_count_offset;
    uint32_t stride;
} rpe_commands_draw_indirect_index_t;

typedef struct PushConstantCommand
{
    void* data;
    size_t size;
    VkShaderStageFlags stage;
} rpe_commands_copy_t;

struct MapBufferCommand
{
    void* data;
    size_t size;
    size_t offset;
    buffer_handle_t handle;
};

struct CondRenderCommand
{
    buffer_handle_t handle;
    int32_t offset;
};

struct PipelineBindCommand
{
    shader_prog_bundle_t* bundle;
};

rpe_cmd_bucket_t* rpe_command_bucket_init(size_t size, arena_t* arena);

rpe_cmd_packet_t* rpe_command_bucket_add_command(
    rpe_cmd_bucket_t* bucket,
    size_t aux_mem_size,
    size_t cmd_size,
    arena_t* arena,
    DispatchFunction func);

rpe_cmd_packet_t* rpe_command_bucket_append_command(
    rpe_cmd_bucket_t* bucket,
    rpe_cmd_packet_t* cmd,
    size_t aux_mem_size,
    size_t cmd_size,
    arena_t* arena,
    DispatchFunction func);

/* ** Private functions. ** */

void rpe_command_bucket_submit(rpe_cmd_bucket_t* bucket, vkapi_driver_t* driver);

void rpe_command_bucket_reset(rpe_cmd_bucket_t* bucket);

rpe_cmd_packet_t* rpe_cmd_packet_create(size_t aux_mem_size, size_t cmd_size, arena_t* arena);

void rpe_cmd_dispatch_draw(vkapi_driver_t* driver, void* data);
void rpe_cmd_dispatch_index_draw(vkapi_driver_t* driver, void* data);
void rpe_cmd_dispatch_draw_indirect_indexed(vkapi_driver_t* driver, void* data);
void rpe_cmd_dispatch_push_constant(vkapi_driver_t* driver, void* data);
void rpe_cmd_dispatch_map_buffer(vkapi_driver_t* driver, void* data);
void rpe_cmd_dispatch_cond_render(vkapi_driver_t* driver, void* data);
void rpe_cmd_dispatch_pline_bind(vkapi_driver_t* driver, void* data);

#endif