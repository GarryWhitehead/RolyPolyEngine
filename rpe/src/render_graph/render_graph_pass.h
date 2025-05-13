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

#ifndef __RPE_RG_RENDER_GRAPH_PASS_H__
#define __RPE_RG_RENDER_GRAPH_PASS_H__

#include "render_graph_handle.h"

#include <utility/maths.h>
#include <vulkan-api/renderpass.h>

// Forward declarations.
typedef struct RenderPassNode rg_render_pass_node_t;
typedef struct VkApiDriver vkapi_driver_t;
typedef struct RenderGraphResource rg_render_graph_resource_t;
typedef struct Engine rpe_engine_t;

typedef void(execute_func)(vkapi_driver_t*, rpe_engine_t*, rg_render_graph_resource_t*, void*);

typedef struct PassAttachment
{
    rg_handle_t colour[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT];
    rg_handle_t depth;
    rg_handle_t stencil;
} rg_pass_attach_t;

typedef union PassAttachmentUnion
{
    rg_pass_attach_t attach;
    rg_handle_t attach_array[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
} rg_attach_union_t;

typedef struct PassDescriptor
{
    rg_attach_union_t attachments;
    math_vec4f clear_col;
    uint8_t samples;
    uint32_t multi_view_count;
    enum LoadClearFlags ds_load_clear_flags[2];
    enum StoreClearFlags ds_store_clear_flags[2];
    vkapi_rt_handle_t rt_handle;
} rg_pass_desc_t;

typedef struct RenderGraphPass
{
    rg_render_pass_node_t* node;
    void* data;
    execute_func* func;
} rg_pass_t;

static inline rg_pass_desc_t rg_pass_desc_init()
{
    rg_pass_desc_t instance = {0};
    instance.clear_col.a = 1.0f;
    instance.ds_load_clear_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_DONTCARE;
    instance.ds_load_clear_flags[1] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_DONTCARE;
    instance.ds_store_clear_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_DONTCARE;
    instance.ds_store_clear_flags[1] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_DONTCARE;

    for (int i = 0; i < VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT; ++i)
    {
        instance.attachments.attach_array[i].id = RG_INVALID_HANDLE;
    }
    return instance;
}

static inline rg_pass_t* rg_pass_init(execute_func func, size_t data_size, arena_t* arena)
{
    rg_pass_t* i = ARENA_MAKE_STRUCT(arena, rg_pass_t, ARENA_ZERO_MEMORY);
    i->func = func;
    i->data = arena_alloc(arena, data_size, data_size, 1, ARENA_ZERO_MEMORY);
    return i;
}

#endif
