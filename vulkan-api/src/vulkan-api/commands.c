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

#include "context.h"
#include "driver.h"
#include "swapchain.h"

#include <string.h>
#include <utility/arena.h>

typedef struct Commands
{
    /// the main command pool - only to be used on the main thread
    VkCommandPool cmd_pool;
    /// Command pools for use when using threaded cmd buffer generation - one pool per thread.
    /// There are two cmd pool arrays - one will be the active cmd pool which is used for
    /// creating secondary command buffers, the other will have secondary command buffers which
    /// are potentially active.
    VkCommandPool threaded_cmd_pools[VKAPI_COMMANDS_MAX_THREAD_COUNT][2];
    /// The currently "active" cmd pool index.
    uint32_t active_cmd_pool;

    uint32_t queue_idx;
    vkapi_cmdbuffer_t* curr_cmd_buffer;
    VkSemaphore curr_signal;

    // current semaphore that has been submitted to the queue
    VkSemaphore submitted_signal;

    // wait semaphore passed by the client
    VkSemaphore ext_signals[VKAPI_COMMANDS_MAX_EXTERNAL_SIGNAL_COUNT];
    uint32_t ext_signal_count;

    VkQueue cmd_queue;

    vkapi_cmdbuffer_t cmd_buffers[VKAPI_MAX_COMMAND_BUFFER_SIZE];
    VkSemaphore signals[VKAPI_MAX_COMMAND_BUFFER_SIZE];

    size_t available_cmd_buffers;

    // Secondary commands for threaded cmd buffers.
    // TODO: Make the number of secondary cmd arrays user definable.
    arena_dyn_array_t secondary_cmds;

    // RenderPass and FrameBuffer when using threaded secondary cmd buffers.
    VkRenderPass inheritance_rpass;
    VkFramebuffer inheritance_fbo;

    // Viewport/Scissor state from fbo - if a renderable states a valid scissor/viewport,
    // then this will be used instead of this state.
    VkRect2D scissor;
    VkViewport viewport;

    // Number of frames until threaded command pools are cleared to prevent them consuming too much
    // memory.
    size_t frames_since_threaded_cmd_pool_reset;

} vkapi_commands_t;

VkCommandPool
_create_cmd_pool(vkapi_context_t* context, uint32_t queue_index, VkCommandPoolCreateFlagBits flags)
{
    VkCommandPool pool;
    VkCommandPoolCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.queueFamilyIndex = queue_index;
    create_info.flags = flags | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(context->device, &create_info, VK_NULL_HANDLE, &pool));
    return pool;
}

VkCommandPool
_get_threaded_cmd_pool(vkapi_context_t* context, vkapi_commands_t* commands, uint32_t idx)
{
    assert(idx < VKAPI_COMMANDS_MAX_THREAD_COUNT);
    uint32_t active_idx = commands->active_cmd_pool;
    if (!commands->threaded_cmd_pools[idx][active_idx])
    {
        commands->threaded_cmd_pools[idx][active_idx] =
            _create_cmd_pool(context, commands->queue_idx, 0);
    }
    return commands->threaded_cmd_pools[idx][active_idx];
}

vkapi_commands_t* vkapi_commands_init(
    vkapi_context_t* context, uint32_t queue_index, VkQueue cmd_queue, arena_t* arena)
{
    assert(context);

    vkapi_commands_t* instance = ARENA_MAKE_STRUCT(arena, vkapi_commands_t, ARENA_ZERO_MEMORY);
    instance->available_cmd_buffers = VKAPI_MAX_COMMAND_BUFFER_SIZE;
    instance->cmd_queue = cmd_queue;
    instance->queue_idx = queue_index;

    instance->cmd_pool =
        _create_cmd_pool(context, queue_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    // create the semaphore for signaling a new frame is ready now
    for (int i = 0; i < VKAPI_MAX_COMMAND_BUFFER_SIZE; ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {0};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK_RESULT(vkCreateSemaphore(
            context->device, &semaphore_create_info, VK_NULL_HANDLE, &instance->signals[i]));
    }

    MAKE_DYN_ARRAY(VkCommandBuffer, arena, 30, &instance->secondary_cmds);

    return instance;
}

void vkapi_commands_destroy(vkapi_context_t* context, vkapi_commands_t* commands)
{
    vkapi_commands_free_cmd_buffers(context, commands);
    vkDestroyCommandPool(context->device, commands->cmd_pool, VK_NULL_HANDLE);

    for (int i = 0; i < VKAPI_MAX_COMMAND_BUFFER_SIZE; ++i)
    {
        vkDestroySemaphore(context->device, commands->signals[i], VK_NULL_HANDLE);
    }
}

vkapi_cmdbuffer_t*
vkapi_commands_get_cmdbuffer(vkapi_context_t* context, vkapi_commands_t* commands)
{
    if (commands->curr_cmd_buffer)
    {
        return commands->curr_cmd_buffer;
    }

    // wait for cmd buffers to finish before getting a new one.
    while (commands->available_cmd_buffers == 0)
    {
        vkapi_commands_free_cmd_buffers(context, commands);
    }

    for (int i = 0; i < VKAPI_MAX_COMMAND_BUFFER_SIZE; ++i)
    {
        vkapi_cmdbuffer_t* cmd = &commands->cmd_buffers[i];
        if (!cmd->instance)
        {
            commands->curr_cmd_buffer = cmd;
            commands->curr_signal = commands->signals[i];
            --commands->available_cmd_buffers;
            break;
        }
    }
    assert(commands->curr_cmd_buffer);
    assert(commands->curr_signal);

    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = commands->cmd_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(
        context->device, &alloc_info, &commands->curr_cmd_buffer->instance));

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkBeginCommandBuffer(commands->curr_cmd_buffer->instance, &begin_info));

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;
    VK_CHECK_RESULT(vkCreateFence(
        context->device, &fence_info, VK_NULL_HANDLE, &commands->curr_cmd_buffer->fence));

    return commands->curr_cmd_buffer;
}

void vkapi_commands_new_frame_sc(vkapi_commands_t* commands)
{
    assert(commands);
    dyn_array_clear(&commands->secondary_cmds);
    commands->inheritance_fbo = VK_NULL_HANDLE;
    commands->inheritance_rpass = VK_NULL_HANDLE;
}

void vkapi_commands_set_viewport_scissor_state(
    vkapi_commands_t* commands, VkRect2D scissor, VkViewport vp)
{
    assert(commands);
    commands->scissor = scissor;
    commands->viewport = vp;
}

uint32_t vkapi_commands_alloc_sc(vkapi_context_t* context, vkapi_commands_t* commands)
{
    assert(context);
    assert(commands);

    uint32_t id = commands->secondary_cmds.size;
    uint32_t thread_id = id % VKAPI_COMMANDS_MAX_THREAD_COUNT;

    VkCommandBuffer sec_cmds;
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = _get_threaded_cmd_pool(context, commands, thread_id);
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    alloc_info.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(context->device, &alloc_info, &sec_cmds));

    DYN_ARRAY_APPEND(&commands->secondary_cmds, &sec_cmds);
    return id;
}

void vkapi_commands_begin_sc(vkapi_context_t* context, vkapi_commands_t* commands, uint32_t id)
{
    assert(context);
    assert(commands);
    assert(id < commands->secondary_cmds.size);
    assert(
        commands->inheritance_fbo &&
        "Before using a secondary command buffer, the fbo must be set.");
    assert(
        commands->inheritance_rpass &&
        "Before using a secondary command buffer, the renderpass must be set.");

    VkCommandBuffer cmds = DYN_ARRAY_GET(VkCommandBuffer, &commands->secondary_cmds, id);

    VkCommandBufferInheritanceInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    info.renderPass = commands->inheritance_rpass;
    info.framebuffer = commands->inheritance_fbo;

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // Specify that the secondary command buffer will be considered to be entirely inside a render
    // pass. The simultaneous use bit allows the cmd buffer to be recorded in multiple passes - is
    // this required?
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT |
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin_info.pInheritanceInfo = &info;
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmds, &begin_info));
}

void vkapi_commands_set_rpass_sc(vkapi_commands_t* commands, VkRenderPass rpass, VkFramebuffer fbo)
{
    assert(rpass);
    assert(fbo);
    commands->inheritance_rpass = rpass;
    commands->inheritance_fbo = fbo;
}

void vkapi_commands_execute_range_sc(vkapi_commands_t* commands, uint32_t first_idx, uint32_t last_idx)
{
    assert(commands);
    assert(commands->curr_cmd_buffer && "No primary command buffer is currently bound");
    assert(commands->secondary_cmds.size > 0 && "No secondary commands to execute.");
    assert(last_idx < commands->secondary_cmds.size);
    assert(last_idx >= first_idx);

    uint32_t size = (last_idx - first_idx) + 1;
    assert(size > 0);
    VkCommandBuffer* ptr = (VkCommandBuffer*)commands->secondary_cmds.data + first_idx;

    vkCmdExecuteCommands(commands->curr_cmd_buffer->instance, size, ptr);
}

void vkapi_commands_execute_sc(vkapi_commands_t* commands)
{
    vkapi_commands_execute_range_sc(commands, 0, commands->secondary_cmds.size);
}

void vkapi_commands_end_sc(vkapi_commands_t* commands, uint32_t id)
{
    assert(commands);
    assert(id < commands->secondary_cmds.size);
    VkCommandBuffer cmds = DYN_ARRAY_GET(VkCommandBuffer, &commands->secondary_cmds, id);
    vkEndCommandBuffer(cmds);
}

void vkapi_commands_free_cmd_buffers(vkapi_context_t* context, vkapi_commands_t* commands)
{
    // wait for all cmdbuffers to finish their work except the current
    // buffer
    VkFence fences[VKAPI_MAX_COMMAND_BUFFER_SIZE];
    uint32_t count = 0;
    for (int i = 0; i < VKAPI_MAX_COMMAND_BUFFER_SIZE; ++i)
    {
        if (commands->cmd_buffers[i].instance &&
            &commands->curr_cmd_buffer->instance != &commands->cmd_buffers[i].instance)
        {
            fences[count++] = commands->cmd_buffers[i].fence;
        }
    }
    if (count)
    {
        VK_CHECK_RESULT(vkWaitForFences(context->device, count, fences, VK_TRUE, UINT64_MAX));
    }

    // Get all currently allocated fences and wait for them to finish
    for (int i = 0; i < VKAPI_MAX_COMMAND_BUFFER_SIZE; ++i)
    {
        vkapi_cmdbuffer_t* cmd = &commands->cmd_buffers[i];
        if (cmd->instance)
        {
            int result = vkWaitForFences(context->device, 1, &cmd->fence, VK_TRUE, 0);
            if (result == VK_SUCCESS)
            {
                vkFreeCommandBuffers(context->device, commands->cmd_pool, 1, &cmd->instance);
                cmd->instance = VK_NULL_HANDLE;
                vkDestroyFence(context->device, cmd->fence, VK_NULL_HANDLE);
                ++commands->available_cmd_buffers;
            }
        }
    }
}

void vkapi_commands_flush(vkapi_context_t* context, vkapi_commands_t* commands)
{
    // nothing to flush if we have no commands
    if (!commands->curr_cmd_buffer)
    {
        return;
    }

    vkEndCommandBuffer(commands->curr_cmd_buffer->instance);

    VkSemaphore wait_signals[VKAPI_COMMANDS_MAX_EXTERNAL_SIGNAL_COUNT + 1];
    uint8_t signal_idx = 0;

    if (commands->submitted_signal)
    {
        wait_signals[signal_idx++] = commands->submitted_signal;
    }
    for (uint32_t i = 0; i < commands->ext_signal_count; ++i)
    {
        wait_signals[signal_idx++] = commands->ext_signals[i];
    }

    VkPipelineStageFlags flags[2] = {
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = signal_idx;
    submit_info.pWaitSemaphores = signal_idx > 0 ? wait_signals : NULL;
    submit_info.pWaitDstStageMask = flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &commands->curr_cmd_buffer->instance;
    submit_info.pSignalSemaphores = &commands->curr_signal;
    submit_info.signalSemaphoreCount = 1;
    VK_CHECK_RESULT(
        vkQueueSubmit(commands->cmd_queue, 1, &submit_info, commands->curr_cmd_buffer->fence));

    commands->curr_cmd_buffer = NULL;
    memset(
        commands->ext_signals, 0, sizeof(VkSemaphore) * VKAPI_COMMANDS_MAX_EXTERNAL_SIGNAL_COUNT);
    commands->ext_signal_count = 0;
    commands->submitted_signal = commands->curr_signal;
}

VkSemaphore vkapi_commands_get_finished_signal(vkapi_commands_t* commands)
{
    VkSemaphore output = commands->submitted_signal;
    commands->submitted_signal = NULL;
    return output;
}

void vkapi_commands_set_ext_wait_signal(vkapi_commands_t* commands, VkSemaphore s)
{
    assert(commands);
    assert(commands->ext_signal_count < VKAPI_COMMANDS_MAX_EXTERNAL_SIGNAL_COUNT);
    commands->ext_signals[commands->ext_signal_count++] = s;
}

VkCommandBuffer vkapi_commands_get_secondary_cmds(vkapi_commands_t* cmds, uint32_t id)
{
    assert(cmds);
    assert(id < cmds->secondary_cmds.size);
    return DYN_ARRAY_GET(VkCommandBuffer, &cmds->secondary_cmds, id);
}

void vkapi_commands_gc(vkapi_commands_t* cmds, vkapi_driver_t* driver)
{
    if (cmds->frames_since_threaded_cmd_pool_reset >=
        VKAPI_COMMANDS_FRAMES_UNTIL_THREADED_CMDPOOL_RESET)
    {
        // Switch to and reset the inactive command pool array as it will be safe to do so now as
        // all secondary command buffers will have finished their work.
        cmds->active_cmd_pool ^= 1;
        uint32_t active_idx = cmds->active_cmd_pool;
        for (int i = 0; i < VKAPI_COMMANDS_MAX_THREAD_COUNT; ++i)
        {
            VkCommandPool pool = cmds->threaded_cmd_pools[i][active_idx];
            if (pool)
            {
                VK_CHECK_RESULT(vkResetCommandPool(
                    driver->context->device,
                    cmds->threaded_cmd_pools[i][active_idx],
                    VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));
            }
        }
        cmds->frames_since_threaded_cmd_pool_reset = 0;
    }
    else
    {
        cmds->frames_since_threaded_cmd_pool_reset++;
    }
}

VkRect2D vkapi_commands_get_scissor_state(vkapi_commands_t* cmds)
{
    assert(cmds);
    return cmds->scissor;
}

VkViewport vkapi_commands_get_viewport_state(vkapi_commands_t* cmds)
{
    assert(cmds);
    return cmds->viewport;
}
