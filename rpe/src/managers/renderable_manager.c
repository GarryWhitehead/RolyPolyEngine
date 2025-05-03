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
#include "rpe/transform_manager.h"
#include "scene.h"
#include "transform_manager.h"
#include "vertex_buffer.h"

#include <stdlib.h>
#include <string.h>
#include <utility/hash.h>
#include <utility/sort.h>


rpe_renderable_t* rpe_renderable_init(arena_t* arena)
{
    rpe_renderable_t* rend = ARENA_MAKE_ZERO_STRUCT(arena, rpe_renderable_t);
    rend->sort_key = UINT32_MAX;
    // rend.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    rend->box = rpe_aabox_init();
    rend->view_layer = 0x2;
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

void rpe_renderable_set_scissor(rpe_renderable_t* r, int32_t x, int32_t y, uint32_t w, uint32_t h)
{
    r->scissor.x = x;
    r->scissor.y = y;
    r->scissor.width = w;
    r->scissor.height = h;
    r->key.scissor = r->scissor;
}

void rpe_renderable_set_viewport(
    rpe_renderable_t* r,
    int32_t x,
    int32_t y,
    uint32_t w,
    uint32_t h,
    float min_depth,
    float max_depth)
{
    r->viewport.rect.x = x;
    r->viewport.rect.y = y;
    r->viewport.rect.width = w;
    r->viewport.rect.height = h;
    r->viewport.min_depth = min_depth;
    r->viewport.max_depth = max_depth;
    r->key.viewport = r->viewport;
}

void rpe_renderable_set_view_layer(rpe_renderable_t* r, uint8_t layer)
{
    assert(r);
    if (layer > RPE_RENDER_QUEUE_MAX_VIEW_LAYER_COUNT)
    {
        log_warn(
            "Layer value of %i is outside max allowed value (%i). Ignoring.",
            layer,
            RPE_RENDER_QUEUE_MAX_VIEW_LAYER_COUNT);
        return;
    }
    r->view_layer = layer;
}

rpe_rend_manager_t* rpe_rend_manager_init(rpe_engine_t* engine, arena_t* arena)
{
    assert(engine);
    rpe_rend_manager_t* m = ARENA_MAKE_ZERO_STRUCT(arena, rpe_rend_manager_t);
    m->comp_manager = rpe_comp_manager_init(arena);
    MAKE_DYN_ARRAY(rpe_renderable_t, arena, 100, &m->renderables);
    MAKE_DYN_ARRAY(rpe_material_t, arena, 100, &m->materials);
    MAKE_DYN_ARRAY(rpe_mesh_t, arena, 100, &m->meshes);
    MAKE_DYN_ARRAY(rpe_vertex_alloc_info_t, arena, 100, &m->vertex_allocations);

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
    assert(renderable);
    assert(renderable->material);
    assert(renderable->mesh_data);
    assert(transform_obj.id != UINT32_MAX);
    assert(rend_obj.id != UINT32_MAX);

    uint32_t material_key =
        murmur2_hash(&renderable->material->material_key, sizeof(struct MaterialKey), 0);
    uint32_t rend_key = murmur2_hash(&renderable->key, sizeof(struct RenderableKey), 0);
    struct SortKey key = {
        .program_id = material_key + rend_key,
        .view_layer = renderable->view_layer,
        .screen_layer = 0,
        .depth = 0};
    renderable->sort_key = rpe_render_queue_create_sort_key(key, RPE_MATERIAL_KEY_SORT_PROGRAM);
    renderable->transform_obj = transform_obj;

    // First, add the object which will give us a free slot.
    uint64_t idx = rpe_comp_manager_add_obj(m->comp_manager, rend_obj);
    ADD_OBJECT_TO_MANAGER(&m->renderables, idx, renderable);
}

bool rpe_rend_manager_remove(rpe_rend_manager_t*rm, rpe_object_t obj)
{
    assert(rm);
    assert(obj.id != RPE_INVALID_OBJECT);
    return rpe_comp_manager_remove(rm->comp_manager, obj);
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

rpe_valloc_handle rpe_rend_manager_alloc_vertex_buffer(rpe_rend_manager_t* m, uint32_t vertex_size)
{
    rpe_vertex_alloc_info_t v_info =
        rpe_vertex_buffer_alloc_vertex_buffer(m->engine->vbuffer, vertex_size);
    rpe_valloc_handle h = {.id = m->vertex_allocations.size };
    DYN_ARRAY_APPEND(&m->vertex_allocations, &v_info);
    return h;
}

rpe_valloc_handle rpe_rend_manager_alloc_index_buffer(rpe_rend_manager_t* m, uint32_t index_size)
{
    rpe_vertex_alloc_info_t i_info =
        rpe_vertex_buffer_alloc_index_buffer(m->engine->vbuffer, index_size);
    rpe_valloc_handle h = {.id = m->vertex_allocations.size };
    DYN_ARRAY_APPEND(&m->vertex_allocations, &i_info);
    return h;
}

rpe_vertex_alloc_info_t get_alloc_info(rpe_rend_manager_t* m, rpe_valloc_handle h)
{
    assert(h.id < m->vertex_allocations.size);
    return DYN_ARRAY_GET(rpe_vertex_alloc_info_t, &m->vertex_allocations, h.id);
}

rpe_mesh_t* rpe_rend_manager_create_mesh(
    rpe_rend_manager_t* m,
    rpe_valloc_handle v_handle,
    rpe_vertex_t* vertex_data,
    uint32_t vertex_size,
    rpe_valloc_handle i_handle,
    void* indices,
    uint32_t indices_size,
    enum IndicesType indices_type,
    enum MeshAttributeFlags mesh_flags)
{
    rpe_engine_t* engine = m->engine;
    rpe_vertex_alloc_info_t v_info = get_alloc_info(m, v_handle);
    rpe_vertex_alloc_info_t i_info = get_alloc_info(m, i_handle);

    assert(vertex_size <= v_info.size);
    assert(indices_size <= i_info.size);

    rpe_mesh_t mesh = {.index_offset = i_info.offset, .vertex_offset = v_info.offset};
    rpe_vertex_buffer_copy_vert_data(engine->vbuffer, v_info, vertex_data);
    if (indices_type == RPE_RENDERABLE_INDICES_U32)
    {
        rpe_vertex_buffer_copy_index_data_u32(engine->vbuffer, i_info, (int32_t*)indices);
    }
    else
    {
        rpe_vertex_buffer_copy_index_data_u16(engine->vbuffer, i_info, (int16_t*)indices);
    }
    mesh.index_count = indices_size;
    mesh.mesh_flags = mesh_flags;
    return DYN_ARRAY_APPEND(&m->meshes, &mesh);
}

rpe_mesh_t* rpe_rend_manager_create_mesh_interleaved(
    rpe_rend_manager_t* m,
    rpe_valloc_handle v_handle,
    float* pos_data,
    float* uv0_data,
    float* uv1_data,
    float* normal_data,
    float* tangent_data,
    float* col_data,
    float* bone_weight_data,
    float* bone_id_data,
    uint32_t vertex_size,
    rpe_valloc_handle i_handle,
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
        memcpy(v->position, pos_ptr, 3 * sizeof(float));
        pos_ptr += 3;

        if (uv0_data)
        {
            memcpy(v->uv0, uv0_ptr, 2 * sizeof(float));
            uv0_ptr += 2;
            mesh_flags |= RPE_MESH_ATTRIBUTE_UV0;
        }
        if (uv1_data)
        {
            memcpy(v->uv1, uv1_ptr, 2 * sizeof(float));
            uv1_ptr += 2;
            mesh_flags |= RPE_MESH_ATTRIBUTE_UV1;
        }
        if (normal_data)
        {
            memcpy(v->normal, normal_ptr, 3 * sizeof(float));
            normal_ptr += 3;
            mesh_flags |= RPE_MESH_ATTRIBUTE_NORMAL;
        }
        if (tangent_data)
        {
            memcpy(v->tangent, tangent_ptr, 4 * sizeof(float));
            tangent_ptr += 4;
            mesh_flags |= RPE_MESH_ATTRIBUTE_TANGENT;
        }
        if (col_data)
        {
            memcpy(v->colour, colour_ptr, 4 * sizeof(float));
            colour_ptr += 4;
            mesh_flags |= RPE_MESH_ATTRIBUTE_COLOUR;
        }
        if (bone_weight_data)
        {
            memcpy(v->bone_weight, weight_ptr, 4 * sizeof(float));
            weight_ptr += 4;
            mesh_flags |= RPE_MESH_ATTRIBUTE_BONE_WEIGHT;
        }
        if (bone_id_data)
        {
            memcpy(v->bone_id, id_ptr, 4 * sizeof(float));
            id_ptr += 4;
            mesh_flags |= RPE_MESH_ATTRIBUTE_BONE_ID;
        }
    }

    rpe_mesh_t* mesh = rpe_rend_manager_create_mesh(
        m, v_handle, tmp, vertex_size, i_handle, indices, indices_size, indices_type, mesh_flags);
    arena_reset(&m->engine->scratch_arena);
    return mesh;
}

rpe_mesh_t* rpe_rend_manager_create_static_mesh(
    rpe_rend_manager_t* m,
    rpe_valloc_handle v_handle,
    float* pos_data,
    float* uv0_data,
    float* normal_data,
    float* col_data,
    uint32_t vertex_size,
    rpe_valloc_handle i_handle,
    void* indices,
    uint32_t indices_size,
    enum IndicesType indices_type)
{
    return rpe_rend_manager_create_mesh_interleaved(
        m,
        v_handle,
        pos_data,
        uv0_data,
        NULL,
        normal_data,
        NULL,
        col_data,
        NULL,
        NULL,
        vertex_size,
        i_handle,
        indices,
        indices_size,
        indices_type);
}

rpe_mesh_t* rpe_rend_manager_offset_indices(
    rpe_rend_manager_t* m, rpe_mesh_t* mesh, uint32_t index_offset, uint32_t index_count)
{
    rpe_mesh_t new_mesh = *mesh;
    new_mesh.index_offset = mesh->index_offset + index_offset;
    new_mesh.index_count = index_count;
    return DYN_ARRAY_APPEND(&m->meshes, &new_mesh);
}

rpe_material_t* rpe_rend_manager_create_material(rpe_rend_manager_t* m, rpe_scene_t* scene)
{
    assert(m);
    assert(scene);
    rpe_engine_t* engine = m->engine;
    rpe_material_t mat = rpe_material_init(engine, scene, &engine->perm_arena);
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

void rpe_rend_manager_batch_renderables(
    rpe_rend_manager_t* m, arena_dyn_array_t* object_arr, arena_dyn_array_t* batched_renderables)
{
    assert(m);

    dyn_array_clear(batched_renderables);
    if (!object_arr->size)
    {
        return;
    }

    QSORT_RS(object_arr->data, object_arr->size, sizeof(rpe_object_t), sort_renderables, (void*)m);

    rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, object_arr, 0);
    rpe_renderable_t* rend = rpe_rend_manager_get_mesh(m, obj);
    rpe_batch_renderable_t batch = {
        .material = rend->material,
        .first_idx = 0,
        .count = 1,
        .scissor = rend->scissor,
        .viewport = rend->viewport};

    rpe_batch_renderable_t* curr_batch = DYN_ARRAY_APPEND(batched_renderables, &batch);
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
            // As the sort key also includes the scissor and viewport which means changes in these
            // params will result in a new batch, we just take the scissor and viewport from the
            // first renderable.
            batch.scissor = rend->scissor;
            batch.viewport = rend->viewport;
            batch.first_idx = i;
            batch.count = 1;
            curr_batch = DYN_ARRAY_APPEND(batched_renderables, &batch);
        }
        prev = rend;
    }
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
