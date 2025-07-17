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

#include "engine.h"

#include <assert.h>
#include <string.h>
#include <utility/arena.h>
#include <utility/job_queue.h>
#include <vulkan-api/commands.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/pipeline_cache.h>
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
    if (diic->id != UINT32_MAX)
    {
        vkapi_driver_draw_indirect_indexed_sc(
            driver,
            diic->cmd_handle,
            diic->offset,
            diic->count_handle,
            diic->draw_count_offset,
            diic->stride,
            diic->max_draw_count,
            diic->id);
    }
    else
    {
        vkapi_driver_draw_indirect_indexed(
            driver,
            diic->cmd_handle,
            diic->offset,
            diic->count_handle,
            diic->draw_count_offset,
            diic->stride,
            diic->max_draw_count);
    }
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
    assert(cmd->bundle);
    if (cmd->id != UINT32_MAX)
    {
        vkapi_driver_bind_gfx_pipeline_sc(driver, cmd->bundle, cmd->id);
    }
    else
    {
        vkapi_driver_bind_gfx_pipeline(driver, cmd->bundle);
    }
}

void rpe_cmd_dispatch_desc_bind(vkapi_driver_t* driver, void* data)
{
    struct DescriptorSetBindCommand* cmd = (struct DescriptorSetBindCommand*)data;
    assert(cmd->bundle);
    if (cmd->id != UINT32_MAX)
    {
        // Force rebinding of descriptor sets when used with secondary buffers as
        // its likely that the descriptor cache is being used across multiple threads
        // and this can mess up the previous bound descriptor set state checks.
        vkapi_driver_bind_descriptors_sc(driver, cmd->bundle, true, cmd->id);
    }
    else
    {
        vkapi_driver_bind_descriptors(driver, cmd->bundle, false);
    }
}

void rpe_cmd_dispatch_scissor_cmd(vkapi_driver_t* driver, void* data)
{
    struct ScissorCommand* cmd = (struct ScissorCommand*)data;
    // If the renderable doesn't set the scissor/viewport, use the state from commands.
    VkRect2D scissor = {0};
    if (cmd->scissor.width > 0)
    {
        scissor.extent.width = cmd->scissor.width;
        scissor.extent.height = cmd->scissor.height;
        scissor.offset.x = cmd->scissor.x;
        scissor.offset.y = cmd->scissor.y;
    }
    else
    {
        scissor = vkapi_commands_get_scissor_state(driver->commands);
        assert(scissor.extent.width > 0);
    }

    if (cmd->id != UINT32_MAX)
    {
        vkapi_driver_set_scissor_sc(driver, scissor, cmd->id);
    }
    else
    {
        vkapi_driver_set_scissor(driver, scissor);
    }
}

void rpe_cmd_dispatch_viewport_cmd(vkapi_driver_t* driver, void* data)
{
    struct ViewportCommand* cmd = (struct ViewportCommand*)data;
    VkViewport vp = {0};
    if (cmd->vp.rect.width > 0)
    {
        vp.width = (float)cmd->vp.rect.width;
        vp.height = (float)cmd->vp.rect.height;
        vp.x = (float)cmd->vp.rect.x;
        vp.y = (float)cmd->vp.rect.y;
        vp.minDepth = cmd->vp.min_depth;
        vp.maxDepth = cmd->vp.max_depth;
    }
    else
    {
        vp = vkapi_commands_get_viewport_state(driver->commands);
        assert(vp.width > 0);
    }

    if (cmd->id != UINT32_MAX)
    {
        vkapi_driver_set_viewport_sc(driver, vp, cmd->id);
    }
    else
    {
        vkapi_driver_set_viewport(driver, vp);
    }
}

void rpe_cmd_dispatch_bind_vertex_buffer(vkapi_driver_t* driver, void* data)
{
    struct BindVertexBufferCommand* cmd = (struct BindVertexBufferCommand*)data;
    if (cmd->id != UINT32_MAX)
    {
        vkapi_driver_bind_vertex_buffer_sc(driver, cmd->handle, cmd->binding, cmd->id);
    }
    else
    {
        vkapi_driver_bind_vertex_buffer(driver, cmd->handle, cmd->binding);
    }
}

void rpe_cmd_dispatch_bind_index_buffer(vkapi_driver_t* driver, void* data)
{
    struct BindIndexBufferCommand* cmd = (struct BindIndexBufferCommand*)data;
    if (cmd->id != UINT32_MAX)
    {
        vkapi_driver_bind_index_buffer_sc(driver, cmd->handle, cmd->id);
    }
    else
    {
        vkapi_driver_bind_index_buffer(driver, cmd->handle);
    }
}

void rpe_cmd_dispatch_begin_secondary_cmds(vkapi_driver_t* driver, void* data)
{
    struct SecondaryCmdBeginCommand* cmd = (struct SecondaryCmdBeginCommand*)data;
    vkapi_commands_begin_sc(driver->context, driver->commands, cmd->id);
}

void rpe_cmd_dispatch_end_secondary_cmds(vkapi_driver_t* driver, void* data)
{
    struct SecondaryCmdEndCommand* cmd = (struct SecondaryCmdEndCommand*)data;
    vkapi_commands_end_sc(driver->commands, cmd->id);
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
    packet->submit_threaded = false;

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

struct PktSubmitData
{
    rpe_cmd_packet_t* pkt;
    vkapi_driver_t* driver;
};

void _pkt_submit_runner(void* data)
{
    struct PktSubmitData* pkt_info = (struct PktSubmitData*)data;
    rpe_cmd_packet_t* pkt = pkt_info->pkt;
    do
    {
        rpe_cmd_packet_submit(pkt, pkt_info->driver);
        pkt = pkt->next;
    } while (pkt);
}

void _submit_bucket(rpe_cmd_packet_t** packets, rpe_engine_t* engine, size_t count)
{
    vkapi_driver_t* driver = engine->driver;
    bool threaded_cmdbuffers = driver->settings.threaded_cmdbuffers;

    struct PktSubmitData per_thread_bucket_data[VKAPI_COMMANDS_MAX_THREAD_COUNT];
    job_t* parent = NULL;

    for (size_t i = 0; i < count; ++i)
    {
        assert(i < VKAPI_COMMANDS_MAX_THREAD_COUNT);

        struct PktSubmitData* data = &per_thread_bucket_data[i];
        data->pkt = packets[i];
        data->driver = engine->driver;

        if (data->pkt->submit_threaded && threaded_cmdbuffers)
        {
            parent = !parent ? job_queue_create_parent_job(engine->job_queue) : parent;
            job_t* job = job_queue_create_job(engine->job_queue, _pkt_submit_runner, data, parent);
            job_queue_run_job(engine->job_queue, job);
        }
        else
        {
            _pkt_submit_runner(&per_thread_bucket_data[i]);
        }
    }

    if (parent)
    {
        // Check all cmd buffer jobs have finished before exiting.
        job_queue_run_and_wait(engine->job_queue, parent);
    }
}

void rpe_command_bucket_submit(rpe_cmd_bucket_t* bucket, rpe_engine_t* engine)
{
    assert(bucket);

    const size_t bucket_count = bucket->curr_index;

    uint32_t iters = bucket_count / VKAPI_COMMANDS_MAX_THREAD_COUNT;
    uint32_t rem = bucket_count % VKAPI_COMMANDS_MAX_THREAD_COUNT;

    rpe_cmd_packet_t** pkt_ptr = bucket->packets;
    for (size_t i = 0; i < iters; ++i, pkt_ptr += VKAPI_COMMANDS_MAX_THREAD_COUNT)
    {
        _submit_bucket(pkt_ptr, engine, VKAPI_COMMANDS_MAX_THREAD_COUNT);
    }
    // Submit any remaining buckets.
    _submit_bucket(pkt_ptr, engine, rem);
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
