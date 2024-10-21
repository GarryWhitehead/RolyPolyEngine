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

#include "commands.h"

#include "context.h"
#include "swapchain.h"

#include <string.h>

typedef struct Commands
{
    /// the main command pool - only to be used on the main thread
    VkCommandPool cmd_pool;

    vkapi_cmdbuffer_t* curr_cmd_buffer;
    VkSemaphore curr_signal;

    // current semaphore that has been submitted to the queue
    VkSemaphore submitted_signal;

    // wait semaphore passed by the client
    VkSemaphore ext_signal;

    vkapi_cmdbuffer_t cmd_buffers[VKAPI_MAX_COMMAND_BUFFER_SIZE];
    VkSemaphore signals[VKAPI_MAX_COMMAND_BUFFER_SIZE];

    size_t available_cmd_buffers;
} vkapi_commands_t;

vkapi_commands_t* vkapi_commands_init(vkapi_context_t* context, arena_t* arena)
{
    assert(context);

    vkapi_commands_t* instance = ARENA_MAKE_STRUCT(arena, vkapi_commands_t, ARENA_ZERO_MEMORY);

    VkCommandPoolCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.queueFamilyIndex = context->queue_info.graphics;
    create_info.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    VK_CHECK_RESULT(
        vkCreateCommandPool(context->device, &create_info, VK_NULL_HANDLE, &instance->cmd_pool));

    // create the semaphore for signalling a new frame is ready now
    for (int i = 0; i < VKAPI_MAX_COMMAND_BUFFER_SIZE; ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {0};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK_RESULT(vkCreateSemaphore(
            context->device, &semaphore_create_info, VK_NULL_HANDLE, &instance->signals[i]));
    }

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

    VkSemaphore wait_signals[2];
    uint8_t signal_idx = 0;

    if (commands->submitted_signal)
    {
        wait_signals[signal_idx++] = commands->submitted_signal;
    }
    if (commands->ext_signal)
    {
        wait_signals[signal_idx++] = commands->ext_signal;
    }

    VkPipelineStageFlags flags[2] = {
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = signal_idx;
    submit_info.pWaitSemaphores = wait_signals;
    submit_info.pWaitDstStageMask = flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &commands->curr_cmd_buffer->instance;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &commands->curr_signal;
    VK_CHECK_RESULT(
        vkQueueSubmit(context->graphics_queue, 1, &submit_info, commands->curr_cmd_buffer->fence));

    commands->curr_cmd_buffer = NULL;
    commands->ext_signal = NULL;
    commands->submitted_signal = commands->curr_signal;
}

VkSemaphore vkapi_commands_get_finished_signal(vkapi_commands_t* commands)
{
    VkSemaphore output = commands->submitted_signal;
    commands->submitted_signal = NULL;
    return output;
}
