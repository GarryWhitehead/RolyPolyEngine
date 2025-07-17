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

#ifndef __VKAPI_COMMANDS_H__
#define __VKAPI_COMMANDS_H__

#include "common.h"
#include "utility/arena.h"
#include "utility/compiler.h"

// This needs to be at least three due to triple buffering - plus a few more frames for good
// measure. Note: Setting this too high on systems with less memory (<2gb), setting this too high
// (>=10) leads to issues with memory exhaustion mainly due to textures being waited on for erasure
// from the frame graph.
#define VKAPI_MAX_COMMAND_BUFFER_SIZE 5
#define VKAPI_COMMANDS_MAX_EXTERNAL_SIGNAL_COUNT 3

#define VKAPI_COMMANDS_MAX_THREAD_COUNT 8

// The cmd pool for each cmd buffer thread is reset on a set frame interval to avoid it consuming too much memory.
#define VKAPI_COMMANDS_FRAMES_UNTIL_THREADED_CMDPOOL_RESET 5

// forward declarations
typedef struct VkApiContext vkapi_context_t;
typedef struct Commands vkapi_commands_t;
typedef struct VkApiDriver vkapi_driver_t;

typedef struct CmdBuffer
{
    VkCommandBuffer instance;
    VkFence fence;

} vkapi_cmdbuffer_t;

vkapi_commands_t* vkapi_commands_init(
    vkapi_context_t* context, uint32_t queue_index, VkQueue cmd_queue, arena_t* arena);

void vkapi_commands_destroy(vkapi_context_t* context, vkapi_commands_t* commands);

vkapi_cmdbuffer_t*
vkapi_commands_get_cmdbuffer(vkapi_context_t* context, vkapi_commands_t* commands);

void vkapi_commands_free_cmd_buffers(vkapi_context_t* context, vkapi_commands_t* commands);

void vkapi_commands_flush(vkapi_context_t* context, vkapi_commands_t* commands);

VkSemaphore vkapi_commands_get_finished_signal(vkapi_commands_t* commands);

void vkapi_commands_set_ext_wait_signal(vkapi_commands_t* commands, VkSemaphore s);

/**** Secondary command buffer functions ****/

void vkapi_commands_new_frame_sc(vkapi_commands_t* commands);

uint32_t vkapi_commands_alloc_sc(vkapi_context_t* context, vkapi_commands_t* commands);

void
vkapi_commands_begin_sc(vkapi_context_t* context, vkapi_commands_t* commands, uint32_t id);

void vkapi_commands_set_rpass_sc(
    vkapi_commands_t* commands, VkRenderPass rpass, VkFramebuffer fbo);

void vkapi_commands_set_viewport_scissor_state(
    vkapi_commands_t* commands, VkRect2D scissor, VkViewport vp);

void vkapi_commands_execute_sc(vkapi_commands_t* commands);

void vkapi_commands_execute_range_sc(vkapi_commands_t* commands, uint32_t first_idx, uint32_t last_idx);

void vkapi_commands_end_sc(vkapi_commands_t* commands, uint32_t id);

VkCommandBuffer vkapi_commands_get_secondary_cmds(vkapi_commands_t* cmds, uint32_t id);

VkRect2D vkapi_commands_get_scissor_state(vkapi_commands_t* cmds);

VkViewport vkapi_commands_get_viewport_state(vkapi_commands_t* cmds);

void vkapi_commands_gc(vkapi_commands_t* cmds, vkapi_driver_t* driver);

#endif