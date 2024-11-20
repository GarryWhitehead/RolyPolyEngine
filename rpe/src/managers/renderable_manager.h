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

#ifndef __RPE_RENDERABLE_MANAGER_H__
#define __RPE_RENDERABLE_MANAGER_H__

#include "aabox.h"
#include "material.h"

#include <utility/arena.h>
#include <vulkan-api/program_manager.h>

typedef struct Engine rpe_engine_t;
typedef struct ComponentManager rpe_comp_manager_t;
typedef struct Object rpe_object_t;

enum Visible
{
    RPE_RENDERABLE_VIS_RENDER,
    // Removes this renderable from the culling process.
    RPE_RENDERABLE_VIS_IGNORE,
    RPE_RENDERABLE_VIS_SHADOW,
    RPE_RENDERABLE_VIS_CULL
};

enum Variants
{
    RPE_RENDERABLE_PRIM_HAS_SKIN = 1 << 0,
    RPE_RENDERABLE_PRIM_HAS_JOINTS = 1 << 1
};

typedef struct RenderPrimitive
{
    VkPrimitiveTopology topology;
    VkBool32 prim_restart;
    // Index offsets
    size_t index_count;
    uint32_t index_offset;
    uint32_t vertex_offset;
    size_t vertex_count;
    // The min and max extents of the primitive
    rpe_aabox_t box;

    uint64_t material_flags;

    // The material for this primitive. This isn't owned by the
    // primitive - this is the "property" of the renderable manager.
    rpe_material_t* material;
} rpe_render_primitive_t;

typedef struct Renderable
{
    // Visibility of this renderable and their shadow.
    uint64_t visibility;

    // ============ vulkan backend ========================
    // tesselation vertices count - if non-zero assumes a tesselation shader pipeline is used.
    uint32_t tesse_vert_count;
    arena_dyn_array_t primitives;
} rpe_renderable_t;

static inline rpe_render_primitive_t*
rpe_renderable_get_primitive(rpe_renderable_t* r, uint32_t idx)
{
    return DYN_ARRAY_GET_PTR(rpe_render_primitive_t, &r->primitives, idx);
}

typedef struct RenderableManager
{
    // the buffers containing all the model data
    arena_dyn_array_t renderables;
    // all the materials
    arena_dyn_array_t materials;

    rpe_comp_manager_t* comp_manager;
} rpe_rend_manager_t;

rpe_renderable_t rpe_renderable_init(arena_t* arena);

rpe_rend_manager_t* rpe_rend_manager_init(arena_t* arena);

void rpe_rend_manager_add(rpe_rend_manager_t* m, rpe_renderable_t* renderable, rpe_object_t obj);

rpe_renderable_t* rpe_rend_manager_get_mesh(rpe_rend_manager_t* m, rpe_object_t* obj);

#endif
