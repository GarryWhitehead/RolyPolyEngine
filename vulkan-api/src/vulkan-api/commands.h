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

#pragma once

#include "common.h"
#include "utility/arena.h"
#include "utility/compiler.h"

// some arbitrary numbers which need monitoring for possible issues due to
// overflow
#define VKAPI_MAX_COMMAND_BUFFER_SIZE 10

// forward declarations
typedef struct VkApiContext vkapi_context_t;

typedef struct CmdBuffer
{
    VkCommandBuffer instance;
    VkFence fence;

} vkapi_cmdbuffer_t;

typedef struct ThreadedCmdBuffer
{
    VkCommandBuffer secondary;
    VkCommandPool cmd_pool;
    bool is_executed;

} vkapi_threaded_cmdbuffer_t;

typedef struct Commands
{
    /// the main command pool - only to be used on the main thread
    VkCommandPool cmd_pool;

    vkapi_cmdbuffer_t* curr_cmd_buffer;
    VkSemaphore* curr_signal;

    // current semaphore that has been submitted to the queue
    VkSemaphore* submitted_signal;

    // wait semaphore passed by the client
    VkSemaphore* ext_signal;

    vkapi_cmdbuffer_t cmd_buffers[VKAPI_MAX_COMMAND_BUFFER_SIZE];
    VkSemaphore signals[VKAPI_MAX_COMMAND_BUFFER_SIZE];

    size_t available_cmd_buffers;
} vkapi_commands_t;

vkapi_commands_t vkapi_commands_init(vkapi_context_t* context);

void vkapi_commands_destroy(vkapi_context_t* context, vkapi_commands_t* commands);

vkapi_cmdbuffer_t*
vkapi_commands_get_cmdbuffer(vkapi_context_t* context, vkapi_commands_t* commands);

void vkapi_commands_free_cmd_buffers(vkapi_context_t* context, vkapi_commands_t* commands);

void vkapi_commands_flush(vkapi_context_t* context, vkapi_commands_t* commands);

VkSemaphore* vkapi_commands_get_finished_signal(vkapi_commands_t* commands);
