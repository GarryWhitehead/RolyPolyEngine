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

#include "renderable_manager.h"

#include "component_manager.h"
#include "engine.h"
#include "render_queue.h"
#include "rpe/object.h"
#include "scene.h"
#include "transform_manager.h"
#include "rpe/transform_manager.h"
#include "vertex_buffer.h"

#include <stdlib.h>
#include <utility/hash.h>
#include <utility/sort.h>
#include <string.h>


rpe_renderable_t rpe_renderable_init()
{
    rpe_renderable_t rend = {0};
    rend.sort_key = UINT32_MAX;
    //rend.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    rend.box = rpe_aabox_init();
    return rend;
}

void rpe_renderable_set_box(rpe_renderable_t* r, rpe_aabox_t* box)
{
    assert(r);
    r->box.min.x = box->min.x;
    r->box.min.y = box->min.y;
    r->box.min.z = box->min.z;
    r->box.max.x = box->max.x;
    r->box.max.y = box->max.y;
    r->box.min.z = box->min.z;
}

void rpe_renderable_set_min_max_dimensions(rpe_renderable_t* r, math_vec3f min, math_vec3f max)
{
    assert(r);
    rpe_aabox_t box = {.min = min, .max = max};
    rpe_renderable_set_box(r, &box);
}

rpe_rend_manager_t* rpe_rend_manager_init(rpe_engine_t* engine, arena_t* arena)
{
    assert(engine);
    rpe_rend_manager_t* m = ARENA_MAKE_ZERO_STRUCT(arena, rpe_rend_manager_t);
    m->comp_manager = rpe_comp_manager_init(arena);
    MAKE_DYN_ARRAY(rpe_renderable_t, arena, 100, &m->renderables);
    MAKE_DYN_ARRAY(rpe_material_t, arena, 100, &m->materials);
    MAKE_DYN_ARRAY(rpe_mesh_t, arena, 100, &m->meshes);
    MAKE_DYN_ARRAY(rpe_batch_renderable_t, arena, 100, &m->batched_renderables);

    m->engine = engine;
    return m;
}

void rpe_rend_manager_add(
    rpe_rend_manager_t* m,
    rpe_renderable_t* renderable,
    rpe_object_t rend_obj,
    rpe_object_t transform_obj)
{
    assert(m);
    assert(transform_obj.id != UINT32_MAX);
    assert(rend_obj.id != UINT32_MAX);

    m->is_dirty = true;
    uint32_t material_key =
        murmur2_hash(&renderable->material->material_key, sizeof(struct MaterialKey), 0);
    struct SortKey key = {
        .program_id = material_key,
        .view_layer = renderable->material->view_layer,
        .screen_layer = 0,
        .depth = 0};
    renderable->sort_key = rpe_render_queue_create_sort_key(key, RPE_MATERIAL_KEY_SORT_PROGRAM);
    renderable->transform_obj = transform_obj;

    // First, add the object which will give us a free slot.
    uint64_t idx = rpe_comp_manager_add_obj(m->comp_manager, rend_obj);
    ADD_OBJECT_TO_MANAGER(&m->renderables, idx, renderable);
}

void rpe_rend_manager_copy(
    rpe_rend_manager_t* rm,
    rpe_transform_manager_t* tm,
    rpe_object_t src_obj,
    rpe_object_t dst_obj,
    rpe_object_t* transform_obj)
{
    assert(rm);
    assert(src_obj.id != UINT32_MAX);
    assert(dst_obj.id != UINT32_MAX);
    uint64_t src_idx = rpe_comp_manager_get_obj_idx(rm->comp_manager, src_obj);
    rpe_renderable_t rend = DYN_ARRAY_GET(rpe_renderable_t, &rm->renderables, src_idx);

    // Override initial transform if specified.
    if (transform_obj)
    {
        rend.transform_obj = *transform_obj;
    }
    uint64_t idx = rpe_comp_manager_add_obj(rm->comp_manager, dst_obj);
    ADD_OBJECT_TO_MANAGER(&rm->renderables, idx, &rend);
}

rpe_mesh_t* rpe_rend_manager_create_mesh(
    rpe_rend_manager_t* m,
    float* pos_data,
    float* uv0_data,
    float* uv1_data,
    float* normal_data,
    float* tangent_data,
    float* col_data,
    float* bone_weight_data,
    float* bone_id_data,
    uint32_t vertex_size,
    void* indices,
    uint32_t indices_size,
    enum IndicesType indices_type)
{
    assert(m);
    assert(pos_data);

    enum MeshAttributeFlags mesh_flags = RPE_MESH_ATTRIBUTE_POSITION;
    rpe_vertex_t* tmp = ARENA_MAKE_ZERO_ARRAY(&m->engine->scratch_arena, rpe_vertex_t, vertex_size);

    float* pos_ptr = pos_data;
    float* uv0_ptr = uv0_data;
    float* uv1_ptr = uv1_data;
    float* normal_ptr = normal_data;
    float* tangent_ptr = tangent_data;
    float* colour_ptr = col_data;
    float* weight_ptr = bone_weight_data;
    float* id_ptr = bone_id_data;

    for (uint32_t i = 0; i < vertex_size; ++i)
    {
        rpe_vertex_t* v = &tmp[i];
        memcpy(v->position.data, pos_ptr, 3 * sizeof(float));
        pos_ptr += 3;

        if (uv0_data)
        {
            memcpy(v->uv0.data, uv0_ptr, 2 * sizeof(float));
            uv0_ptr += 2;
            mesh_flags |= RPE_MESH_ATTRIBUTE_UV0;
        }
        if (uv1_data)
        {
            memcpy(v->uv1.data, uv1_ptr, 2 * sizeof(float));
            uv1_ptr += 2;
            mesh_flags |= RPE_MESH_ATTRIBUTE_UV1;
        }
        if (normal_data)
        {
            memcpy(v->normal.data, normal_ptr, 3 * sizeof(float));
            normal_ptr += 3;
            mesh_flags |= RPE_MESH_ATTRIBUTE_NORMAL;
        }
        if (tangent_data)
        {
            memcpy(v->tangent.data, tangent_ptr, 4 * sizeof(float));
            tangent_ptr += 4;
            mesh_flags |= RPE_MESH_ATTRIBUTE_TANGENT;
        }
        if (col_data)
        {
            memcpy(v->colour.data, colour_ptr, 4 * sizeof(float));
            colour_ptr += 4;
            mesh_flags |= RPE_MESH_ATTRIBUTE_COLOUR;
        }
        if (bone_weight_data)
        {
            memcpy(v->bone_weight.data, weight_ptr, 4 * sizeof(float));
            weight_ptr += 4;
            mesh_flags |= RPE_MESH_ATTRIBUTE_BONE_WEIGHT;
        }
        if (bone_id_data)
        {
            memcpy(v->bone_id.data, id_ptr, 4 * sizeof(float));
            id_ptr += 4;
            mesh_flags |= RPE_MESH_ATTRIBUTE_BONE_ID;
        }
    }

    rpe_mesh_t mesh;
    mesh.vertex_offset = rpe_vertex_buffer_copy_vert_data(m->engine->vbuffer, vertex_size, tmp);
    if (indices_type == RPE_RENDERABLE_INDICES_U32)
    {
        mesh.index_offset = rpe_vertex_buffer_copy_index_data_u32(
            m->engine->vbuffer, indices_size, (int32_t*)indices);
    }
    else
    {
        mesh.index_offset = rpe_vertex_buffer_copy_index_data_u16(
            m->engine->vbuffer, indices_size, (int16_t*)indices);
    }
    mesh.index_count = indices_size;
    mesh.mesh_flags = mesh_flags;
    arena_reset(&m->engine->scratch_arena);
    return DYN_ARRAY_APPEND(&m->meshes, &mesh);
}

rpe_mesh_t* rpe_rend_manager_create_static_mesh(
    rpe_rend_manager_t* m,
    float* pos_data,
    float* uv0_data,
    float* normal_data,
    float* col_data,
    uint32_t vertex_size,
    void* indices,
    uint32_t indices_size,
    enum IndicesType indices_type)
{
    return rpe_rend_manager_create_mesh(
        m,
        pos_data,
        uv0_data,
        NULL,
        normal_data,
        NULL,
        col_data,
        NULL,
        NULL,
        vertex_size,
        indices,
        indices_size,
        indices_type);
}

rpe_material_t* rpe_rend_manager_create_material(rpe_rend_manager_t* m)
{
    rpe_material_t mat = rpe_material_init(m->engine, &m->engine->perm_arena);
    return DYN_ARRAY_APPEND(&m->materials, &mat);
}

rpe_renderable_t* rpe_rend_manager_get_mesh(rpe_rend_manager_t* m, rpe_object_t* obj)
{
    assert(m);
    uint64_t idx = rpe_comp_manager_get_obj_idx(m->comp_manager, *obj);
    assert(idx != UINT64_MAX);
    return DYN_ARRAY_GET_PTR(rpe_renderable_t, &m->renderables, idx);
}

#ifdef __linux__ 
int sort_renderables(const void* a, const void* b, void* rend_manager)
#elif WIN32
int sort_renderables(void* rend_manager, const void* a, const void* b)
#endif
{
    assert(rend_manager);
    rpe_object_t* a_obj = (rpe_object_t*)a;
    rpe_object_t* b_obj = (rpe_object_t*)b;
    rpe_rend_manager_t* rm = (rpe_rend_manager_t*)rend_manager;
    rpe_renderable_t* a_rend = rpe_rend_manager_get_mesh(rm, a_obj);
    rpe_renderable_t* b_rend = rpe_rend_manager_get_mesh(rm, b_obj);
    assert(a_rend->sort_key != UINT32_MAX);
    assert(b_rend->sort_key != UINT32_MAX);
    if (a_rend->sort_key > b_rend->sort_key)
    {
        return 1;
    }
    if (a_rend->sort_key < b_rend->sort_key)
    {
        return -1;
    }
    return 0;
}

arena_dyn_array_t*
rpe_rend_manager_batch_renderables(rpe_rend_manager_t* m, arena_dyn_array_t* object_arr)
{
    assert(m);
    if (!object_arr->size || !m->is_dirty)
    {
        return &m->batched_renderables;
    }

    dyn_array_clear(&m->batched_renderables);
    QSORT_RS(object_arr->data, object_arr->size, sizeof(rpe_object_t), sort_renderables, (void*)m);

    rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, object_arr, 0);
    rpe_renderable_t* rend = rpe_rend_manager_get_mesh(m, obj);
    rpe_batch_renderable_t batch = {.material = rend->material, .first_idx = 0, .count = 1};
    rpe_batch_renderable_t* curr_batch = DYN_ARRAY_APPEND(&m->batched_renderables, &batch);
    rpe_renderable_t* prev = rend;

    for (size_t i = 1; i < object_arr->size; ++i)
    {
        obj = DYN_ARRAY_GET_PTR(rpe_object_t, object_arr, i);
        rend = rpe_rend_manager_get_mesh(m, obj);

        if (rend->sort_key == prev->sort_key)
        {
            ++curr_batch->count;
        }
        else
        {
            batch.material = rend->material;
            batch.first_idx = i;
            batch.count = 1;
            curr_batch = DYN_ARRAY_APPEND(&m->batched_renderables, &batch);
        }

        prev = rend;
    }

    m->is_dirty = false;
    return &m->batched_renderables;
}

bool rpe_rend_manager_has_obj(rpe_rend_manager_t* m, rpe_object_t* obj)
{
    assert(m);
    return rpe_comp_manager_has_obj(m->comp_manager, *obj);
}

rpe_object_t rpe_rend_manager_get_transform(rpe_rend_manager_t* rm, rpe_object_t rend_obj)
{
    assert(rm);
    assert(rend_obj.id != UINT32_MAX);
    uint64_t src_idx = rpe_comp_manager_get_obj_idx(rm->comp_manager, rend_obj);
    rpe_renderable_t rend = DYN_ARRAY_GET(rpe_renderable_t, &rm->renderables, src_idx);
    return rend.transform_obj;
}
