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

#include "transform_manager.h"

#include "engine.h"

rpe_transform_manager_t* rpe_transform_manager_init(arena_t* arena)
{
    rpe_transform_manager_t* m = ARENA_MAKE_ZERO_STRUCT(arena, rpe_transform_manager_t);
    MAKE_DYN_ARRAY(rpe_transform_node_t, arena, 100, &m->nodes);
    MAKE_DYN_ARRAY(rpe_skin_instance_t, arena, 100, &m->skins);
    m->comp_manager = rpe_comp_manager_init(arena);
    return m;
}

bool rpe_transform_manager_add_root_node(
    rpe_transform_manager_t* m,
    rpe_transform_node_t* node,
    rpe_object_t obj,
    rpe_skin_instance_t* skin)
{
    rpe_transform_params_t params;
    params.nodes[0] = *node;
    params.nodes[0].parent = NULL;
    params.node_next_idx = 1;

    // Add skins to the manager - these don't require a slot to be requested as
    // there may be numerous skins per mesh. Instead, the starting index of this
    // group will be used to offset the skin indices to point at the correct
    // skin.
    if (skin)
    {
        params.skin_offset = m->skins.size;
        DYN_ARRAY_APPEND(&m->skins, skin);
    }

    // Update the model transform, and if skinned, joint matrices.
    rpe_transform_manager_update_model_transform(m, &params.nodes[0], &params);

    // Request a slot for this object.
    rpe_obj_handle_t h = rpe_comp_manager_add_obj(m->comp_manager, obj);

    ADD_OBJECT_TO_MANAGER(&m->nodes, h.id, &params)
    return true;
}

void rpe_transform_manager_add_chid_node(
    rpe_transform_manager_t* m, rpe_transform_node_t* child, rpe_object_t obj, uint32_t node_idx)
{
    assert(node_idx < RPE_TRANSFORM_MANAGER_MAX_NODE_COUNT);
    rpe_obj_handle_t handle = rpe_comp_manager_get_obj_idx(m->comp_manager, obj);
    rpe_transform_params_t* p = DYN_ARRAY_GET_PTR(rpe_transform_params_t, &m->nodes, handle.id);
    p->nodes[p->node_next_idx] = *child;
    rpe_transform_node_t* n = &p->nodes[node_idx];
    DYN_ARRAY_APPEND(&n->children, &p->node_next_idx);
    ++p->node_next_idx;
}

math_mat4f rpe_transform_manager_update_matrix(rpe_transform_node_t* n)
{
    assert(n);

    math_mat4f mat = n->node_transform;
    rpe_transform_node_t* parent = n->parent;
    while (parent)
    {
        mat = math_mat4f_mul(&parent->node_transform, &mat);
        parent = parent->parent;
    }
    return mat;
}

void rpe_transform_manager_update_model_transform(
    rpe_transform_manager_t* m, rpe_transform_node_t* parent, rpe_transform_params_t* params)
{
    assert(m);
    assert(parent);

    // We need to find the mesh node first - we will then update matrices
    // working back towards the root node
    if (parent->has_mesh)
    {
        math_mat4f mat = rpe_transform_manager_update_matrix(parent);

        // add updated local transform to the ubo buffer
        params->model_transform = mat;

        if (params->skin_offset != UINT32_MAX)
        {
            // The transform data index for this Object is stored on the
            // component.
            assert(params->skin_offset >= 0);
            rpe_skin_instance_t* skin =
                DYN_ARRAY_GET_PTR(rpe_skin_instance_t, &m->skins, params->skin_offset);

            uint32_t joint_count =
                MIN(skin->joint_nodes.size, RPE_TRANSFORM_MANAGER_MAX_BONE_COUNT);

            // Transform to local space.
            math_mat4f inv_m = math_mat4f_inverse(&mat);

            for (uint32_t i = 0; i < joint_count; ++i)
            {
                rpe_transform_node_t* joint_node =
                    DYN_ARRAY_GET_PTR(rpe_transform_node_t, &skin->joint_nodes, i);

                math_mat4f joint_mat = rpe_transform_manager_update_matrix(joint_node);
                joint_mat = math_mat4f_mul(
                    &joint_mat, DYN_ARRAY_GET_PTR(math_mat4f, &skin->inv_bind_matrices, i));

                // Transform joint to local (joint) space.
                math_mat4f local_matrix = math_mat4f_mul(&inv_m, &joint_mat);
                DYN_ARRAY_SET(&params->joint_matrices, i, &local_matrix);
            }
        }

        // one mesh per node is required. So don't bother with the child nodes
        return;
    }

    // Now work up the child nodes - until we find a mesh.
    for (size_t i = 0; i < parent->children.size; ++i)
    {
        rpe_transform_node_t* child = DYN_ARRAY_GET_PTR(rpe_transform_node_t, &parent->children, i);
        rpe_transform_manager_update_model_transform(m, child, params);
    }
}

void rpe_transform_manager_update_model(rpe_transform_manager_t* m, rpe_object_t obj)
{
    assert(m);
    rpe_obj_handle_t handle = rpe_comp_manager_get_obj_idx(m->comp_manager, obj);
    rpe_transform_params_t* p = DYN_ARRAY_GET_PTR(rpe_transform_params_t, &m->nodes, handle.id);
    rpe_transform_manager_update_model_transform(m, &p->nodes[0], p);
}

rpe_transform_params_t*
rpe_transform_manager_get_transform(rpe_transform_manager_t* m, rpe_object_t obj)
{
    rpe_obj_handle_t handle = rpe_comp_manager_get_obj_idx(m->comp_manager, obj);
    assert(handle.id <= m->nodes.size);
    return DYN_ARRAY_GET_PTR(rpe_transform_params_t, &m->nodes, handle.id);
}
