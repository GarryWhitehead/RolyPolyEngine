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

#ifndef __RPE_PRIV_LIGHT_MANAGER_H__
#define __RPE_PRIV_LIGHT_MANAGER_H__

#include "component_manager.h"
#include "rpe/light_manager.h"
#include <utility/maths.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/resource_cache.h>

#include <stdint.h>

typedef struct Engine rpe_engine_t;
typedef struct Scene rpe_scene_t;
typedef struct ShaderProgramBundle shader_prog_bundle_t;
typedef struct ComponentManager rpe_comp_manager_t;
typedef struct Camera rpe_camera_t;

#define RPE_LIGHTING_SAMPLER_MAX_LIGHT_COUNT 50
#define RPE_LIGHTING_SAMPLER_END_OF_BUFFER_SIGNAL 0xFF

#define RPE_LIGHT_MANAGER_CAMERA_UBO_BINDING 0

typedef struct LightInstance
{
    enum LightType type;

    // set by visibility checks
    bool is_visible;

    // Set by a call to update.
    math_mat4f mvp;
    math_vec3f position;
    math_vec3f target;
    math_vec3f colour;
    float fov;
    float intensity;

    struct SpotLightInfo
    {
        float scale;
        float offset;
        float cos_outer_sq;
        float outer;
        float radius;
    } spot_light_info;
} rpe_light_instance_t;

// This must mirror the lighting struct on the shader.
struct LightSsbo
{
    math_mat4f mvp;
    math_vec4f pos;
    math_vec4f direction;
    math_vec4f colour;
    int type;
    float fall_out;
    float scale;
    float offset;
};

typedef struct LightManager
{
    struct LightingConstants
    {
        bool has_ibl;
        uint32_t light_count;

    } light_consts;

    rpe_engine_t* engine;

    arena_dyn_array_t lights;

    // Used for generating the ssbo light data per frame.
    struct LightSsbo ssbo_buffers[RPE_LIGHTING_SAMPLER_MAX_LIGHT_COUNT + 1];

    // keep track of the scene the light manager was last prepared for
    rpe_scene_t* current_scene;

    // if a directional light is set then keep track of its object
    // as the light parameters are also held by the scene ubo
    rpe_object_t dir_light_obj;

    float sun_angular_radius;
    float sun_halo_size;
    float sun_halo_falloff;

    rpe_comp_manager_t* comp_manager;

    // ================= vulkan backend =======================

    shader_prog_bundle_t* program_bundle;
    buffer_handle_t ssbo_vk_buffer_handle;
    shader_handle_t shaders[2];

} rpe_light_manager_t;

rpe_light_manager_t* rpe_light_manager_init(rpe_engine_t* engine, arena_t* arena);

rpe_light_instance_t*
rpe_light_manager_get_light_instance(rpe_light_manager_t* lm, rpe_object_t obj);

void rpe_light_manager_update(rpe_light_manager_t* lm, rpe_scene_t* scene, rpe_camera_t* camera);

rpe_light_instance_t* rpe_light_manager_get_dir_light_params(rpe_light_manager_t* lm);

#endif