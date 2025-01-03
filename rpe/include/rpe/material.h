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
#ifndef __RPE_MATERIAL_H__
#define __RPE_MATERIAL_H__

#include <backend/enums.h>
#include <stdbool.h>
#include <utility/maths.h>

typedef struct Material rpe_material_t;

struct MaterialBlendFactor
{
    bool state;
    enum BlendFactor src_color;
    enum BlendFactor dstColor;
    enum BlendOp colour;
    enum BlendFactor srcAlpha;
    enum BlendFactor dstAlpha;
    enum BlendOp alpha;
};

void rpe_material_set_blend_factors(rpe_material_t* m, struct MaterialBlendFactor factors);

void rpe_material_set_double_sided_state(rpe_material_t* m, bool state);
void rpe_material_set_test_enable(rpe_material_t* m, bool state);
void rpe_material_set_write_enable(rpe_material_t* m, bool state);
void rpe_material_set_depth_compare_op(rpe_material_t* m, enum CompareOp op);
void rpe_material_set_polygon_mode(rpe_material_t* m, enum PolygonMode mode);
void rpe_material_set_front_face(rpe_material_t* m, enum FrontFace face);
void rpe_material_set_cull_mode(rpe_material_t* m, enum CullMode mode);

void rpe_material_set_view_layer(rpe_material_t* m, uint8_t layer);

void rpe_material_set_base_colour_factor(rpe_material_t* m, math_vec4f* f);
void rpe_material_set_diffuse_factor(rpe_material_t* m, math_vec4f* f);
void rpe_material_set_bas_colour_factor(rpe_material_t* m, math_vec4f* f);
void rpe_material_set_emissive_factor(rpe_material_t* m, math_vec4f* f);
void rpe_material_set_roughness_factor(rpe_material_t* m, float f);
void rpe_material_set_metallic_factor(rpe_material_t* m, float f);
void rpe_material_set_alpha_mask(rpe_material_t* m, float mask);
void rpe_material_set_alpha_cutoff(rpe_material_t* m, float co);

#endif