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

#include "renderable_manager.h"

#include "component_manager.h"
#include "engine.h"
#include "render_queue.h"
#include "rpe/object.h"
#include "scene.h"
#include "transform_manager.h"
#include "vertex_buffer.h"

#include <stdlib.h>
#include <utility/hash.h>


rpe_renderable_t rpe_renderable_init()
{
    rpe_renderable_t rend = {0};
    rend.sort_key = UINT32_MAX;
    rend.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    rend.box = rpe_aabox_init();
    return rend;
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

void rpe_rend_manager_add(rpe_rend_manager_t* m, rpe_renderable_t* renderable, rpe_object_t obj)
{
    assert(m);

    m->is_dirty = true;
    uint32_t material_key =
        murmur_hash3(&renderable->material->material_key, sizeof(struct MaterialKey));
    struct SortKey key = {
        .program_id = material_key,
        .view_layer = renderable->material->view_layer,
        .screen_layer = 0,
        .depth = 0};
    renderable->sort_key = rpe_render_queue_create_sort_key(key, RPE_MATERIAL_KEY_SORT_PROGRAM);

    // First add the Object which will give us a free slot.
    uint64_t idx = rpe_comp_manager_add_obj(m->comp_manager, obj);
    ADD_OBJECT_TO_MANAGER(&m->renderables, idx, renderable);
}

rpe_mesh_t* rpe_rend_manager_create_mesh(
    rpe_rend_manager_t* m,
    math_vec3f* pos_data,
    math_vec2f* uv_data,
    math_vec3f* normal_data,
    math_vec4f* col_data,
    math_vec4f* bone_weight_data,
    math_vec4f* bone_id_data,
    uint32_t vertex_size,
    int32_t* indices,
    uint32_t indices_size)
{
    assert(m);
    assert(pos_data);

    enum MeshAttributeFlags mesh_flags = RPE_MESH_ATTRIBUTE_POSITION;
    rpe_vertex_t* tmp = ARENA_MAKE_ZERO_ARRAY(&m->engine->scratch_arena, rpe_vertex_t, vertex_size);
    for (uint32_t i = 0; i < vertex_size; ++i)
    {
        rpe_vertex_t* v = &tmp[i];
        v->position = pos_data[i];
        if (uv_data)
        {
            v->uv = uv_data[i];
            mesh_flags |= RPE_MESH_ATTRIBUTE_UV;
        }
        if (normal_data)
        {
            v->normal = normal_data[i];
            mesh_flags |= RPE_MESH_ATTRIBUTE_NORMAL;
        }
        if (col_data)
        {
            v->colour = col_data[i];
            mesh_flags |= RPE_MESH_ATTRIBUTE_COLOUR;
        }
        if (bone_weight_data)
        {
            v->bone_weight = bone_weight_data[i];
            mesh_flags |= RPE_MESH_ATTRIBUTE_BONE_WEIGHT;
        }
        if (bone_id_data)
        {
            v->bone_id = bone_id_data[i];
            mesh_flags |= RPE_MESH_ATTRIBUTE_BONE_ID;
        }
    }

    rpe_mesh_t mesh;
    mesh.vertex_offset = rpe_vertex_buffer_copy_vert_data(m->engine->vbuffer, vertex_size, tmp);
    mesh.index_offset =
        rpe_vertex_buffer_ccpy_index_data(m->engine->vbuffer, indices_size, indices);
    mesh.index_count = indices_size;
    mesh.mesh_flags = mesh_flags;
    arena_reset(&m->engine->scratch_arena);
    return DYN_ARRAY_APPEND(&m->meshes, &mesh);
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

int sort_renderables(const void* a, const void* b, void* rend_manager)
{
    assert(rend_manager);
    rpe_object_t* a_obj = (rpe_object_t*)a;
    rpe_object_t* b_obj = (rpe_object_t*)b;
    rpe_rend_manager_t* r_manager = (rpe_rend_manager_t*)rend_manager;
    rpe_renderable_t* a_rend = rpe_rend_manager_get_mesh(r_manager, a_obj);
    rpe_renderable_t* b_rend = rpe_rend_manager_get_mesh(r_manager, b_obj);
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
    qsort_r(object_arr->data, object_arr->size, sizeof(rpe_object_t), sort_renderables, (void*)m);

    rpe_object_t* obj = DYN_ARRAY_GET_PTR(rpe_object_t, object_arr, 0);
    rpe_renderable_t* rend = rpe_rend_manager_get_mesh(m, obj);
    rpe_batch_renderable_t batch = {.material = rend->material, .first_idx = 0, .count = 1};
    rpe_batch_renderable_t* curr_batch = DYN_ARRAY_APPEND(&m->batched_renderables, &batch);
    rpe_renderable_t* prev = rend;

    for (size_t i = 1; i < object_arr->size; ++i)
    {
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
