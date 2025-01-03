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

#include "commands.h"

#include <assert.h>
#include <string.h>
#include <utility/arena.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/shader.h>

void rpe_cmd_dispatch_draw(vkapi_driver_t* driver, void* data)
{
    struct DrawCommand* dc = (struct DrawCommand*)data;
    vkapi_driver_draw(driver, dc->vertex_count, dc->start_vertex);
}

void rpe_cmd_dispatch_index_draw(vkapi_driver_t* driver, void* data)
{
    struct DrawIndexCommand* dc = (struct DrawIndexCommand*)data;
    vkapi_driver_draw_indexed(driver, dc->index_count, dc->vertex_offset, dc->index_offset);
}

void rpe_cmd_dispatch_draw_indirect_indexed(vkapi_driver_t* driver, void* data)
{
    struct DrawIndirectIndexCommand* diic = (struct DrawIndirectIndexCommand*)data;
    vkapi_driver_draw_indirect_indexed(
        driver,
        diic->cmd_handle,
        diic->offset,
        diic->count_handle,
        diic->draw_count_offset,
        diic->stride);
}

void rpe_cmd_dispatch_push_constant(vkapi_driver_t* driver, void* data)
{
    struct PushConstantCommand* cmd = (struct PushConstantCommand*)data;
    vkapi_driver_set_push_constant(driver, cmd->data, cmd->size, cmd->stage);
}

void rpe_cmd_dispatch_map_buffer(vkapi_driver_t* driver, void* data)
{
    struct MapBufferCommand* cmd = (struct MapBufferCommand*)data;
    vkapi_driver_map_gpu_buffer(driver, cmd->handle, cmd->size, cmd->offset, cmd->data);
}

void rpe_cmd_dispatch_cond_render(vkapi_driver_t* driver, void* data)
{
    struct CondRenderCommand* cmd = (struct CondRenderCommand*)data;
    vkapi_driver_begin_cond_render(driver, cmd->handle, cmd->offset);
}

void rpe_cmd_dispatch_pline_bind(vkapi_driver_t* driver, void* data)
{
    struct PipelineBindCommand* cmd = (struct PipelineBindCommand*)data;
    vkapi_driver_bind_gfx_pipeline(driver, cmd->bundle);
}

rpe_cmd_bucket_t* rpe_command_bucket_init(size_t size, arena_t* arena)
{
    rpe_cmd_bucket_t* bkt = ARENA_MAKE_ZERO_STRUCT(arena, rpe_cmd_bucket_t);
    bkt->packets = ARENA_MAKE_ZERO_ARRAY(arena, rpe_cmd_packet_t*, size);
    bkt->curr_index = 0;
    return bkt;
}

rpe_cmd_packet_t* rpe_command_bucket_add_command(
    rpe_cmd_bucket_t* bucket,
    size_t aux_mem_size,
    size_t cmd_size,
    arena_t* arena,
    DispatchFunction func)
{
    assert(bucket);
    rpe_cmd_packet_t* packet = rpe_cmd_packet_create(aux_mem_size, cmd_size, arena);

    // store key and pointer to the data
    uint32_t idx = bucket->curr_index++;
    bucket->packets[idx] = packet;

    packet->next = NULL;
    packet->dispatch_func = func;

    return packet;
}

rpe_cmd_packet_t* rpe_command_bucket_append_command(
    rpe_cmd_bucket_t* bucket,
    rpe_cmd_packet_t* prev_pkt,
    size_t data_size,
    size_t cmd_size,
    arena_t* arena,
    DispatchFunction func)
{
    assert(bucket);
    rpe_cmd_packet_t* packet = rpe_cmd_packet_create(data_size, cmd_size, arena);

    prev_pkt->next = packet;
    packet->next = NULL;
    packet->dispatch_func = func;

    return packet;
}

void rpe_cmd_packet_submit(rpe_cmd_packet_t* pkt, vkapi_driver_t* driver)
{
    assert(pkt);
    DispatchFunction func = pkt->dispatch_func;
    void* cmd = pkt->cmds;
    func(driver, cmd);
}

void rpe_command_bucket_submit(rpe_cmd_bucket_t* bucket, vkapi_driver_t* driver)
{
    assert(bucket);

    for (size_t i = 0; i < bucket->curr_index; ++i)
    {
        rpe_cmd_packet_t* pkt = bucket->packets[i];
        do
        {
            rpe_cmd_packet_submit(pkt, driver);
            pkt = pkt->next;
        } while (pkt);
    }
}

void rpe_command_bucket_reset(rpe_cmd_bucket_t* bucket)
{
    assert(bucket);
    bucket->curr_index = 0;
}

rpe_cmd_packet_t* rpe_cmd_packet_create(size_t data_size, size_t cmd_size, arena_t* arena)
{
    size_t pkt_size = sizeof(struct CommandPacket) + data_size * sizeof(uint8_t);
    uint8_t* bytes = ARENA_MAKE_ZERO_ARRAY(arena, uint8_t, pkt_size);
    rpe_cmd_packet_t* pkt = (rpe_cmd_packet_t*)bytes;
    pkt->cmds = ARENA_MAKE_ZERO_ARRAY(arena, uint8_t, cmd_size);
    return pkt;
}
