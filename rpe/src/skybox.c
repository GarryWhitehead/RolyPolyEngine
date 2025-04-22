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

#include "skybox.h"

#include "engine.h"
#include "ibl.h"
#include "managers/object_manager.h"
#include "managers/renderable_manager.h"
#include "managers/transform_manager.h"
#include "rpe/engine.h"
#include "rpe/material.h"
#include "rpe/object_manager.h"
#include "rpe/transform_manager.h"

rpe_skybox_t* rpe_skybox_init(rpe_engine_t* engine, arena_t* arena)
{
    rpe_skybox_t* skybox = ARENA_MAKE_ZERO_STRUCT(arena, rpe_skybox_t);
    skybox->obj = rpe_obj_manager_create_obj(engine->obj_manager);
    skybox->material = rpe_material_init(engine, arena);

    rpe_material_set_cull_mode(&skybox->material, RPE_CULL_MODE_FRONT);
    rpe_material_set_view_layer(&skybox->material, 0x4);
    rpe_material_set_type(&skybox->material, RPE_MATERIAL_SKYBOX);
    rpe_material_set_test_enable(&skybox->material, true);
    rpe_material_set_write_enable(&skybox->material, true);
    rpe_material_set_depth_compare_op(&skybox->material, RPE_COMPARE_OP_LESS_OR_EQUAL);

    // Upload cube vertices.
    // clang-format off
    math_vec3f cube_vertices[8] = {
        {-1.0f, -1.0f, 1.0f},  {1.0f, -1.0f, 1.0f},
        {1.0f,  1.0f, 1.0f},   {-1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f},
        {1.0f, 1.0f, -1.0f},   {-1.0f, 1.0f,  -1.0f}};

    // cube indices
    uint32_t cube_indices[36] = {
        0, 1, 2, 2, 3, 0,       // front
        1, 5, 6, 6, 2, 1,       // right side
        7, 6, 5, 5, 4, 7,       // left side
        4, 0, 3, 3, 7, 4,       // bottom
        4, 5, 1, 1, 0, 4,       // back
        3, 2, 6, 6, 7, 3        // top
    };
    // clang-format on

    skybox->cube_mesh = rpe_rend_manager_create_mesh_interleaved(
        engine->rend_manager,
        (float*)cube_vertices,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        8,
        cube_indices,
        36,
        RPE_RENDERABLE_INDICES_U32);

    return skybox;
}

void rpe_skybox_set_cubemap_from_ibl(rpe_skybox_t* sb, ibl_t* ibl, rpe_engine_t* engine)
{
    assert(sb);
    assert(ibl);
    assert(vkapi_tex_handle_is_valid(ibl->tex_cube_map));
    sb->cube_texture = ibl->tex_cube_map;

    rpe_material_set_device_texture(
        &sb->material, ibl->tex_cube_map, RPE_MATERIAL_IMAGE_TYPE_BASE_COLOR, 0);

    math_mat4f local_transform = math_mat4f_identity();
    rpe_transform_manager_add_node(engine->transform_manager, &local_transform, NULL, &sb->obj);

    rpe_renderable_t* renderable =
        rpe_engine_create_renderable(engine, &sb->material, sb->cube_mesh);
    rpe_rend_manager_add(engine->rend_manager, renderable, sb->obj, sb->obj);
}
