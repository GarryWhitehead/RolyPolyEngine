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
#include "scene.h"
#include "rpe/object.h"

rpe_renderable_t rpe_renderable_init(arena_t* arena)
{
    rpe_renderable_t r;
    memset(&r, 0, sizeof(rpe_renderable_t));
    r.visibility = RPE_RENDERABLE_VIS_CULL;
    MAKE_DYN_ARRAY(rpe_render_primitive_t, arena, 50, &r.primitives);
    return r;
}

rpe_rend_manager_t* rpe_rend_manager_init(arena_t* arena)
{
    rpe_rend_manager_t* m = ARENA_MAKE_ZERO_STRUCT(arena, rpe_rend_manager_t);
    m->comp_manager = rpe_comp_manager_init(arena);
    MAKE_DYN_ARRAY(rpe_renderable_t, arena, 100, &m->renderables);
    MAKE_DYN_ARRAY(rpe_material_t, arena, 100, &m->materials);
    return m;
}

void rpe_rend_manager_add(rpe_rend_manager_t* m, rpe_renderable_t* renderable, rpe_object_t obj)
{
    assert(m);
    for (size_t i = 0; i < renderable->primitives.size; ++i)
    {
        rpe_render_primitive_t* prim = DYN_ARRAY_GET_PTR(rpe_render_primitive_t, &renderable->primitives, i);
        assert(prim->material && "Material must be initialised for a render primitive.");
        rpe_material_set_render_prim(prim->material, prim);
    }

    // First add the Object which will give us a free slot.
    rpe_obj_handle_t h = rpe_comp_manager_add_obj(m->comp_manager, obj);

    ADD_OBJECT_TO_MANAGER(&m->renderables, h.id, renderable)
}

rpe_renderable_t* rpe_rend_manager_get_mesh(rpe_rend_manager_t* m, rpe_object_t* obj)
{
    assert(m);
    rpe_obj_handle_t h = rpe_comp_manager_get_obj_idx(m->comp_manager, *obj);
    assert(rpe_obj_handle_is_valid(h));
    return DYN_ARRAY_GET_PTR(rpe_renderable_t, &m->renderables, h.id);
}