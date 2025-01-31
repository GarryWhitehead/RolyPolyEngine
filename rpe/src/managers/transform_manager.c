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
#include "rpe/transform_manager.h"
#include "scene.h"

rpe_model_transform_t rpe_model_transform_init()
{
    rpe_model_transform_t i = {.scale.x = 1.0f, .scale.y = 1.0f, .scale.z = 1.0f};
    return i;
}

rpe_transform_node_t rpe_transform_node_init()
{
    rpe_transform_node_t n = {0};
    n.world_transform = math_mat4f_identity();
    n.local_transform = math_mat4f_identity();
    return n;
}

rpe_transform_manager_t* rpe_transform_manager_init(rpe_engine_t* engine, arena_t* arena)
{
    assert(engine);

    rpe_transform_manager_t* m = ARENA_MAKE_ZERO_STRUCT(arena, rpe_transform_manager_t);
    MAKE_DYN_ARRAY(rpe_transform_node_t, arena, 100, &m->nodes);
    MAKE_DYN_ARRAY(rpe_skin_instance_t, arena, 100, &m->skins);
    m->static_transforms = ARENA_MAKE_ARRAY(arena, math_mat4f, RPE_SCENE_MAX_STATIC_MODEL_COUNT, 0);
    m->skinned_transforms = ARENA_MAKE_ARRAY(arena, math_mat4f, RPE_SCENE_MAX_BONE_COUNT, 0);
    m->comp_manager = rpe_comp_manager_init(arena);
    m->engine = engine;

    m->bone_buffer_handle = vkapi_res_cache_create_ssbo(
        engine->driver->res_cache,
        engine->driver,
        sizeof(math_mat4f) * RPE_SCENE_MAX_BONE_COUNT,
        0,
        VKAPI_BUFFER_HOST_TO_GPU);
    m->transform_buffer_handle = vkapi_res_cache_create_ssbo(
        engine->driver->res_cache,
        engine->driver,
        sizeof(math_mat4f) * RPE_SCENE_MAX_STATIC_MODEL_COUNT,
        0,
        VKAPI_BUFFER_HOST_TO_GPU);

    return m;
}

void rpe_transform_manager_add_node(
    rpe_transform_manager_t* m,
    math_mat4f* local_transform,
    rpe_object_t* parent_obj,
    rpe_object_t* child_obj)
{
    assert(m);

    rpe_transform_node_t child_node = {
        .local_transform = *local_transform,
        .world_transform = *local_transform,
        .parent = parent_obj};

    if (parent_obj)
    {
        uint64_t parent_idx = rpe_comp_manager_get_obj_idx(m->comp_manager, *parent_obj);
        rpe_transform_node_t* parent_node =
            DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, parent_idx);
        child_node.next = parent_node->first_child;
        parent_node->first_child = child_obj;
    }

    // Update the model transform.
    rpe_transform_manager_update_world(m, &child_node);

    // Request a slot for this object.
    uint64_t idx = rpe_comp_manager_add_obj(m->comp_manager, *child_obj);

    ADD_OBJECT_TO_MANAGER(&m->nodes, idx, &child_node);
    m->is_dirty = true;
}

void rpe_transform_manager_add_local_transform(
    rpe_transform_manager_t* m, rpe_model_transform_t* transform, rpe_object_t* obj)
{
    math_mat4f mat = math_mat4f_identity();
    math_mat4f_translate(transform->translation, &mat);
    math_mat4f_from_mat3f(transform->rot, &mat);
    math_mat4f_scale(transform->scale, &mat);
    rpe_transform_manager_add_node(m, &mat, NULL, obj);
}

void update_world_children(rpe_transform_manager_t* m, rpe_object_t* child)
{
    while (child)
    {
        uint32_t child_idx = rpe_comp_manager_get_obj_idx(m->comp_manager, *child);
        rpe_transform_node_t* child_node =
            DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, child_idx);

        uint32_t parent_idx = rpe_comp_manager_get_obj_idx(m->comp_manager, *child_node->parent);
        rpe_transform_node_t* parent_node =
            DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, parent_idx);

        child_node->world_transform =
            math_mat4f_mul(parent_node->world_transform, child_node->local_transform);

        if (child_node->first_child)
        {
            update_world_children(m, child_node->first_child);
        }
        child = child_node->next;
    }
}

void rpe_transform_manager_update_world(rpe_transform_manager_t* m, rpe_transform_node_t* node)
{
    assert(node);

    rpe_object_t* parent = node->parent;
    if (parent)
    {
        uint32_t parent_idx = rpe_comp_manager_get_obj_idx(m->comp_manager, *parent);
        rpe_transform_node_t* parent_node =
            DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, parent_idx);
        node->world_transform = math_mat4f_mul(parent_node->world_transform, node->local_transform);

        update_world_children(m, node->first_child);
    }
}

/*void rpe_transform_manager_update_model_transform(
    rpe_transform_manager_t* m, rpe_transform_node_t* parent, rpe_transform_params_t* params)
{
    assert(m);
    assert(parent);

    // We need to find the mesh node first - we will then update matrices
    // working back towards the root node
    if (parent->has_mesh)
    {
        math_mat4f mat = rpe_transform_manager_update_matrix(parent);

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
            math_mat4f inv_m = math_mat4f_inverse(mat);

            for (uint32_t i = 0; i < joint_count; ++i)
            {
                rpe_transform_node_t* joint_node =
                    DYN_ARRAY_GET_PTR(rpe_transform_node_t, &skin->joint_nodes, i);

                math_mat4f joint_mat = rpe_transform_manager_update_matrix(joint_node);
                joint_mat = math_mat4f_mul(
                    joint_mat, DYN_ARRAY_GET(math_mat4f, &skin->inv_bind_matrices, i));

                // Transform joint to local (joint) space.
                math_mat4f local_matrix = math_mat4f_mul(inv_m, joint_mat);
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
}*/

void rpe_transform_manager_update_model(rpe_transform_manager_t* m, rpe_object_t obj)
{
    assert(m);
    uint64_t idx = rpe_comp_manager_get_obj_idx(m->comp_manager, obj);
    assert(idx != UINT64_MAX);
    rpe_transform_node_t* node = DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, idx);
    rpe_transform_manager_update_world(m, node);
}

rpe_transform_node_t* rpe_transform_manager_get_node(rpe_transform_manager_t* m, rpe_object_t obj)
{
    uint64_t idx = rpe_comp_manager_get_obj_idx(m->comp_manager, obj);
    assert(idx != UINT64_MAX);
    assert(idx <= m->nodes.size);
    return DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, idx);
}

void rpe_transform_manager_update_ssbo(rpe_transform_manager_t* m)
{
    if (!m->is_dirty)
    {
        return;
    }

    // uint32_t bone_count = 0;
    for (size_t i = 0; i < m->nodes.size; ++i)
    {
        rpe_transform_node_t* node = DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, i);
        m->static_transforms[i] = node->world_transform;
        /*if (p->joint_matrices.size > 0)
        {
            assert(bone_count < RPE_SCENE_MAX_BONE_COUNT);
            // TODO: This needs checking if it actually works.
            memcpy(
                m->skinned_transforms + bone_count,
                p->joint_matrices.data,
                sizeof(math_mat4f) * p->joint_matrices.size);
            bone_count += p->joint_matrices.size;
        }*/
    }

    vkapi_driver_map_gpu_buffer(
        m->engine->driver,
        m->transform_buffer_handle,
        m->nodes.size * sizeof(math_mat4f),
        0,
        m->static_transforms->data);

    /*if (bone_count > 0)
    {
        vkapi_driver_map_gpu_buffer(
            m->engine->driver,
            m->bone_buffer_handle,
            bone_count * sizeof(math_mat4f),
            0,
            m->skinned_transforms->data);
    }*/
    m->is_dirty = false;
}
