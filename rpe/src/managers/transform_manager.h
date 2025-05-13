/* Copyright (c) 2024-2025 Garry Whitehead
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

#ifndef __RPE_PRIV_TRANSFORM_MANAGER_H__
#define __RPE_PRIV_TRANSFORM_MANAGER_H__

#include "component_manager.h"

#include <utility/arena.h>
#include <utility/maths.h>
#include <utility/string.h>
#include <vulkan-api/buffer.h>
#include <vulkan-api/resource_cache.h>

#define RPE_TRANSFORM_MANAGER_MAX_BONE_COUNT 25
#define RPE_TRANSFORM_MANAGER_MAX_NODE_COUNT 500

typedef struct Engine rpe_engine_t;

typedef struct SkinInstance
{
    // links the bone node name with the inverse transform
    arena_dyn_array_t inv_bind_matrices;
    // a list of joints - poiints to the node in the skeleon hierachy which will
    // be transformed
    arena_dyn_array_t joint_nodes;
} rpe_skin_instance_t;

typedef struct TransformNode
{
    /// A flag indicating whether this node contains a mesh
    /// the mesh is actually stored outside the hierarchy.
    bool has_mesh;
    // The transform matrix transformed by the parent matrix.
    math_mat4f local_transform;
    // The world transform matrix for this node.
    math_mat4f world_transform;
    // Parent of this node. NULL signifies the root.
    rpe_object_t* parent;
    rpe_object_t* first_child;
    rpe_object_t* next;
} rpe_transform_node_t;

typedef struct TransformManager
{
    rpe_engine_t* engine;

    math_mat4f* skinned_transforms;
    math_mat4f* static_transforms;
    buffer_handle_t bone_buffer_handle;
    buffer_handle_t transform_buffer_handle;

    // transform data preserved in the node hierarchical format
    // referenced by associated Object
    arena_dyn_array_t nodes;

    // skinned data - inverse bind matrices and bone info
    arena_dyn_array_t skins;

    rpe_component_manager_t* comp_manager;
    bool is_dirty;
} rpe_transform_manager_t;

rpe_transform_manager_t* rpe_transform_manager_init(rpe_engine_t* engine, arena_t* arena);

rpe_transform_node_t* rpe_transform_manager_get_node(rpe_transform_manager_t* m, rpe_object_t obj);

void rpe_transform_manager_update_ssbo(rpe_transform_manager_t* m);

#endif
