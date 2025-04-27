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
#include "rpe/object_manager.h"
#include "rpe/transform_manager.h"
#include "scene.h"

rpe_model_transform_t rpe_model_transform_init()
{
    rpe_model_transform_t i = {.scale.x = 1.0f, .scale.y = 1.0f, .scale.z = 1.0f};
    i.rot.data[0][0] = 1.0f;
    i.rot.data[1][1] = 1.0f;
    i.rot.data[2][2] = 1.0f;
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

     // Request a slot for this object.
    uint64_t idx = rpe_comp_manager_add_obj(m->comp_manager, *child_obj);
    ADD_OBJECT_TO_MANAGER(&m->nodes, idx, &child_node);

    // Update the model transform.
    rpe_transform_manager_update_world(m, *child_obj);

    m->is_dirty = true;
}

void rpe_transform_manager_add_local_transform(
    rpe_transform_manager_t* m, rpe_model_transform_t* transform, rpe_object_t* obj)
{
    math_mat4f T = math_mat4f_identity();
    math_mat4f R = math_mat4f_identity();
    math_mat4f S = math_mat4f_identity();
    math_mat4f_translate(transform->translation, &T);
    math_mat4f_from_mat3f(transform->rot, &R);
    math_mat4f_scale(transform->scale, &S);
    math_mat4f TRS = math_mat4f_mul(T, math_mat4f_mul(R, S));
    rpe_transform_manager_add_node(m, &TRS, NULL, obj);
}

void rpe_transform_manager_insert_node(
    rpe_transform_manager_t* m, rpe_object_t* new_obj, rpe_object_t* parent_obj)
{
    assert(m);
    assert(new_obj->id != UINT32_MAX);
    assert(parent_obj->id != UINT32_MAX);

    uint64_t parent_idx = rpe_comp_manager_get_obj_idx(m->comp_manager, *parent_obj);
    rpe_transform_node_t* parent_node =
        DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, parent_idx);

    uint64_t new_idx = rpe_comp_manager_get_obj_idx(m->comp_manager, *new_obj);
    rpe_transform_node_t* new_node = DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, new_idx);

    parent_node->first_child = new_obj;
    new_node->parent = parent_obj;
    new_node->next = parent_node->first_child;
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

void rpe_transform_manager_update_world(rpe_transform_manager_t* m, rpe_object_t obj)
{
    uint32_t child_idx = rpe_comp_manager_get_obj_idx(m->comp_manager, obj);
    rpe_transform_node_t* node = DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, child_idx);

    rpe_object_t* parent = node->parent;
    if (parent)
    {
        uint32_t parent_idx = rpe_comp_manager_get_obj_idx(m->comp_manager, *parent);
        rpe_transform_node_t* parent_node =
            DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, parent_idx);
        node->world_transform = math_mat4f_mul(parent_node->world_transform, node->local_transform);
    }
    else
    {
        node->world_transform = node->local_transform;
    }
    update_world_children(m, node->first_child);
    m->is_dirty = true;
}

void copy_child_nodes(
    rpe_transform_manager_t* tm,
    rpe_obj_manager_t* om,
    rpe_object_t* child_obj,
    rpe_object_t* parent_obj,
    rpe_transform_node_t* parent_node,
    arena_dyn_array_t* objects)
{
    while (child_obj)
    {
        uint32_t child_idx = rpe_comp_manager_get_obj_idx(tm->comp_manager, *child_obj);
        rpe_transform_node_t* child_node =
            DYN_ARRAY_GET_PTR(rpe_transform_node_t, &tm->nodes, child_idx);

        rpe_object_t new_child_obj = rpe_obj_manager_create_obj(om);
        rpe_object_t* new_child_obj_ptr = DYN_ARRAY_APPEND(objects, &new_child_obj);
        uint32_t new_child_idx = rpe_comp_manager_add_obj(tm->comp_manager, new_child_obj);

        rpe_transform_node_t new_child_node = rpe_transform_node_init();
        new_child_node.world_transform = child_node->world_transform;
        new_child_node.local_transform = child_node->local_transform;
        new_child_node.parent = parent_obj;
        rpe_transform_node_t* new_child_node_ptr =
            ADD_OBJECT_TO_MANAGER_UNSAFE(&tm->nodes, new_child_idx, &new_child_node);

        parent_node->first_child = new_child_obj_ptr;

        if (child_node->first_child)
        {
            copy_child_nodes(
                tm, om, parent_node->first_child, new_child_obj_ptr, new_child_node_ptr, objects);
        }
        child_obj = child_node->next;
    }
}

rpe_object_t rpe_transform_manager_copy(
    rpe_transform_manager_t* tm,
    rpe_obj_manager_t* om,
    rpe_object_t* parent_obj,
    arena_dyn_array_t* objects)
{
    rpe_object_t new_parent_obj = rpe_obj_manager_create_obj(om);
    rpe_object_t* new_parent_obj_ptr = DYN_ARRAY_APPEND(objects, &new_parent_obj);

    uint32_t new_parent_idx = rpe_comp_manager_add_obj(tm->comp_manager, new_parent_obj);

    uint32_t parent_idx = rpe_comp_manager_get_obj_idx(tm->comp_manager, *parent_obj);
    rpe_transform_node_t* parent_node =
        DYN_ARRAY_GET_PTR(rpe_transform_node_t, &tm->nodes, parent_idx);

    rpe_transform_node_t new_parent_node = rpe_transform_node_init();
    new_parent_node.world_transform = parent_node->world_transform;
    new_parent_node.local_transform = parent_node->local_transform;

    rpe_transform_node_t* new_parent_node_ptr =
        ADD_OBJECT_TO_MANAGER_UNSAFE(&tm->nodes, new_parent_idx, &new_parent_node);

    copy_child_nodes(
        tm, om, parent_node->first_child, new_parent_obj_ptr, new_parent_node_ptr, objects);
       
    tm->is_dirty = true;
    return new_parent_obj;
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

rpe_object_t* rpe_transform_manager_get_parent(rpe_transform_manager_t* m, rpe_object_t obj)
{
    assert(m);
    uint64_t idx = rpe_comp_manager_get_obj_idx(m->comp_manager, obj);
    assert(idx != UINT64_MAX);
    rpe_transform_node_t* node = DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, idx);
    return node->parent;
}

void rpe_transform_manager_set_translation(
    rpe_transform_manager_t* m, rpe_object_t obj, math_vec3f trans)
{
    assert(m);
    uint64_t idx = rpe_comp_manager_get_obj_idx(m->comp_manager, obj);
    assert(idx != UINT64_MAX);
    rpe_transform_node_t* node = DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, idx);
    math_mat4f_translate(trans, &node->local_transform);
    rpe_transform_manager_update_world(m, obj);
}

rpe_object_t* rpe_transform_manager_get_child(rpe_transform_manager_t* m, rpe_object_t obj)
{
    assert(m);
    uint64_t idx = rpe_comp_manager_get_obj_idx(m->comp_manager, obj);
    assert(idx != UINT64_MAX);
    rpe_transform_node_t* node = DYN_ARRAY_GET_PTR(rpe_transform_node_t, &m->nodes, idx);
    return node->first_child;
}
