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

#ifndef __RPE_RENDER_QUEUE_H__
#define __RPE_RENDER_QUEUE_H__

#define RPE_RENDER_QUEUE_GBUFFER_SIZE 2048
#define RPE_RENDER_QUEUE_LIGHTING_SIZE 2048
#define RPE_RENDER_QUEUE_POST_PROCESS_SIZE 2048
#define RPE_RENDER_QUEUE_MAX_VIEW_LAYER_COUNT 6

#define VIEW_LAYER_BIT_SHIFT 56
#define SCREEN_LAYER_BIT_SHIFT (VIEW_LAYER_BIT_SHIFT - 8)
#define DEPTH_BIT_SHIFT (SCREEN_LAYER_BIT_SHIFT - 16)
#define PROGRAM_BIT_SHIFT (SCREEN_LAYER_BIT_SHIFT - 16)

#include <stdint.h>

typedef struct CommandBucket rpe_cmd_bucket_t;
typedef struct VkApiDriver vkapi_driver_t;
typedef struct Arena arena_t;

enum SortKeyType
{
    MATERIAL_KEY_SORT_PROGRAM,
    MATERIAL_KEY_SORT_DEPTH
};

/**
 Material key layout:

 64 ------------------------------------ 0
  | v s dddddddddddd ppppppppppppppppppp |

 */
typedef struct MaterialSortKey
{
    uint32_t program_id;        // 4 bytes
    uint8_t screen_layer;       // 1 byte
    uint8_t view_layer;         // 1 byte
    uint16_t depth;             // 2 byte
} material_sort_key_t;

typedef struct RenderQueue
{
    rpe_cmd_bucket_t* gbuffer_bucket;
    rpe_cmd_bucket_t* lighting_bucket;
    rpe_cmd_bucket_t* post_process_bucket;

} rpe_render_queue_t;

rpe_render_queue_t* rpe_render_queue_init(arena_t* arena);

void rpe_render_queue_submit(rpe_render_queue_t* q, vkapi_driver_t* driver);

void rpe_render_queue_clear(rpe_render_queue_t* q);

#endif