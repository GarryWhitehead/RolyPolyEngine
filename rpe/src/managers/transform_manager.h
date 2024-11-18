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

#ifndef __RPE_TRANSFORM_MANAGER_H__
#define __RPE_TRANSFORM_MANAGER_H__

#include "component_manager.h"
#include "component_manager.h"

#include <vulkan-api/buffer.h>
#include <utility/maths.h>
#include <utility/arena.h>
#include <utility/string.h>

#define RPE_TRANSFORM_MANAGER_MAX_BONE_COUNT 25
#define RPE_TRANSFORM_MANAGER_MAX_NODE_COUNT 500

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
    /// The id of the node is derived from the index - and is used to locate
    /// this node if it's a joint or animation target
    string_t id;
    /// a flag indicating whether this node contains a mesh
    /// the mesh is actaully stored outside the hierachy
    bool has_mesh;
    // the transform matrix transformed by the parent matrix
    math_mat4f local_transform;
    // the transform matrix for this node = T*R*S
    math_mat4f node_transform;
    // parent of this node. NULL signifies the root
    struct TransformNode* parent;
    // children of this node
    arena_dyn_array_t children;
} rpe_transform_node_t;

typedef struct TransformParams
{
    rpe_transform_node_t nodes[RPE_TRANSFORM_MANAGER_MAX_NODE_COUNT];
    uint32_t node_next_idx;
    // the transform of this model - calculated by calling updateTransform()
    math_mat4f model_transform;
    // The offset all skin indices will be adjusted by within this
    // node hierachy. We use this also to signify if this model has a skin.
    uint32_t skin_offset;
    // skinning data - set by calling updateTransform()
    arena_dyn_array_t joint_matrices;
} rpe_transform_params_t;

typedef struct TransformManager
{
    // transform data preserved in the node hierarchical format
    // referenced by associated Object
    arena_dyn_array_t nodes;

    // skinned data - inverse bind matrices and bone info
    arena_dyn_array_t skins;

    rpe_component_manager_t* comp_manager;
} rpe_transform_manager_t;

rpe_transform_manager_t* rpe_transform_manager_init(arena_t* arena);

math_mat4f rpe_transform_manager_update_matrix(rpe_transform_node_t* n);

void rpe_transform_manager_update_model_transform(rpe_transform_manager_t* m, rpe_transform_node_t* parent, rpe_transform_params_t* params);

void rpe_transform_manager_update_model(rpe_transform_manager_t* m, rpe_object_t obj);

rpe_transform_params_t*
rpe_transform_manager_get_transform(rpe_transform_manager_t* m, rpe_object_t obj);

#endif
