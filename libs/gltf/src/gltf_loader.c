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

#include "gltf/gltf_loader.h"

#include "gltf/gltf_asset.h"

#include <log.h>
#include <rpe/engine.h>
#include <rpe/material.h>
#include <rpe/object.h>
#include <rpe/object_manager.h>
#include <rpe/renderable_manager.h>
#include <rpe/scene.h>
#include <rpe/transform_manager.h>
#include <utility/arena.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <jsmn.h>
#include <string.h>

struct ExtensionInstance
{
    string_t name;
    string_t values;
};

struct GltfExtensions
{
    struct ExtensionInstance* instances;
    uint32_t instance_count;
};

math_vec3f gltf_extension_token_to_vec3(string_t* str, arena_t* arena)
{
    math_vec3f out;
    uint32_t count;
    string_t** split = string_split(str, ',', &count, arena);
    assert(count == 3 && "String must be of vec3 type");
    out.x = strtof(split[0]->data, NULL);
    out.y = strtof(split[1]->data, NULL);
    out.z = strtof(split[2]->data, NULL);
    return out;
}

struct ExtensionInstance* gltf_extension_find(const char* ext_name, arena_dyn_array_t* exts)
{
    for (size_t i = 0; i < exts->size; ++i)
    {
        struct ExtensionInstance* e = DYN_ARRAY_GET_PTR(struct ExtensionInstance, exts, i);
        if (strcmp(e->name.data, ext_name) == 0)
        {
            return e;
        }
    }
    return NULL;
}

struct GltfExtensions* gltf_extension_build(cgltf_extras* extras, cgltf_data* data, arena_t* arena)
{
    // First check whether there are any extensions.
    cgltf_size ext_size;
    cgltf_copy_extras_json(data, extras, NULL, &ext_size);
    if (!ext_size)
    {
        return NULL;
    }

    char* json_data = ARENA_MAKE_ZERO_ARRAY(arena, char, ext_size + 1);
    cgltf_result res = cgltf_copy_extras_json(data, extras, json_data, &ext_size);
    if (res != cgltf_result_success)
    {
        log_error("Unable to prepare extension data. Error code: %d", res);
        return false;
    }

    json_data[ext_size] = '\0';

    // Create json file from data blob - using cgltf implementation here.
    jsmn_parser json_parser;
    jsmn_init(&json_parser);
    const int token_count = jsmn_parse(&json_parser, json_data, ext_size + 1, NULL, 0);
    if (token_count < 1)
    {
        // Not an error - just no extension data for this model.
        return NULL;
    }

    struct GltfExtensions* out = ARENA_MAKE_ZERO_STRUCT(arena, struct GltfExtensions);
    out->instances = ARENA_MAKE_ZERO_ARRAY(arena, struct ExtensionInstance, ext_size);
    out->instance_count = ext_size;

    // We know the token size, so allocate memory for this and get the tokens.
    jsmntok_t* tokens;
    int r = jsmn_parse(&json_parser, json_data, ext_size, tokens, token_count);
    assert(r % 2 == 0);

    // Now convert everything to a string for easier storage.
    for (int i = 0; i < token_count; i += 2)
    {
        struct ExtensionInstance* instance = &out->instances[i];
        instance->name = string_init_index(json_data, tokens[i].start, tokens[i].end, arena);
        instance->values =
            string_init_index(json_data, tokens[i + 1].start, tokens[i + 1].end, arena);
    }

    return out;
}

uint8_t* gltf_model_get_attr_data(cgltf_attribute* attrib, size_t* stride)
{
    const cgltf_accessor* accessor = attrib->data;
    if (!accessor->buffer_view)
    {
        return NULL;
    }

    *stride = !accessor->buffer_view->stride ? accessor->stride : accessor->buffer_view->stride;
    assert(*stride);
    assert(accessor->component_type == cgltf_component_type_r_32f);
    return (uint8_t*)accessor->buffer_view->buffer->data + accessor->offset +
        accessor->buffer_view->offset;
}

float gltf_model_material_convert_to_alpha(cgltf_alpha_mode mode)
{
    float result;
    switch (mode)
    {
        case cgltf_alpha_mode_opaque:
            result = 0.0f;
            break;
        case cgltf_alpha_mode_mask:
            result = 1.0f;
            break;
        case cgltf_alpha_mode_blend:
            result = 2.0f;
            break;
    }
    return result;
}

rpe_material_t* create_material_instance(cgltf_material* mat, gltf_asset_t* asset)
{
    assert(asset);

    rpe_rend_manager_t* r_manager = rpe_engine_get_rend_manager(asset->engine);
    // TODO: Its annoying having a model tied (non-transparently) to a scene.
    rpe_scene_t* scene = rpe_engine_get_current_scene(asset->engine);
    assert(scene);
    rpe_material_t* new_mat = rpe_rend_manager_create_material(r_manager, scene);

    // Some assumed defaults for the material.
    rpe_material_set_test_enable(new_mat, true);
    rpe_material_set_write_enable(new_mat, true);
    rpe_material_set_depth_compare_op(new_mat, RPE_COMPARE_OP_LESS);
    rpe_material_set_front_face(new_mat, RPE_FRONT_FACE_COUNTER_CLOCKWISE);
    rpe_material_set_cull_mode(new_mat, RPE_CULL_MODE_BACK);

    // If the gltf has no material defined then use the defaults.
    if (!mat)
    {
        return new_mat;
    }

    // Two pipelines, either specular glossiness or metallic roughness,
    // according to the spec, metallic roughness should be preferred.
    if (mat->has_pbr_specular_glossiness)
    {
        rpe_material_set_pipeline(new_mat, RPE_MATERIAL_PIPELINE_SPECULAR);

        if (mat->pbr_specular_glossiness.diffuse_texture.texture)
        {
            asset_texture_t diffuse_asset = {
                .mat = new_mat,
                .gltf_tex = &mat->pbr_specular_glossiness.diffuse_texture,
                .uv_index = mat->pbr_specular_glossiness.diffuse_texture.texcoord,
                .tex_type = RPE_MATERIAL_IMAGE_TYPE_DIFFUSE};
            DYN_ARRAY_APPEND(&asset->textures, &diffuse_asset);
        }

        // instead of having a separate entry for mr or specular gloss, the
        // two share the same slot. Should maybe be renamed to be more
        // transparent?
        if (mat->pbr_specular_glossiness.specular_glossiness_texture.texture)
        {
            asset_texture_t mr_asset = {
                .mat = new_mat,
                .gltf_tex = &mat->pbr_specular_glossiness.specular_glossiness_texture,
                .uv_index = mat->pbr_specular_glossiness.specular_glossiness_texture.texcoord,
                .tex_type = RPE_MATERIAL_IMAGE_TYPE_METALLIC_ROUGHNESS};
            DYN_ARRAY_APPEND(&asset->textures, &mr_asset);
        }

        cgltf_float* df = mat->pbr_specular_glossiness.diffuse_factor;
        math_vec4f vec = {df[0], df[1], df[2], df[3]};
        rpe_material_set_diffuse_factor(new_mat, &vec);
    }
    else if (mat->has_pbr_metallic_roughness)
    {
        rpe_material_set_pipeline(new_mat, RPE_MATERIAL_PIPELINE_MR);

        if (mat->pbr_metallic_roughness.base_color_texture.texture)
        {
            asset_texture_t bc_asset = {
                .mat = new_mat,
                .gltf_tex = &mat->pbr_metallic_roughness.base_color_texture,
                .uv_index = mat->pbr_metallic_roughness.base_color_texture.texcoord,
                .tex_type = RPE_MATERIAL_IMAGE_TYPE_BASE_COLOR};
            DYN_ARRAY_APPEND(&asset->textures, &bc_asset);
        }

        if (mat->pbr_metallic_roughness.metallic_roughness_texture.texture)
        {
            asset_texture_t mr_asset = {
                .mat = new_mat,
                .gltf_tex = &mat->pbr_metallic_roughness.metallic_roughness_texture,
                .uv_index = mat->pbr_metallic_roughness.metallic_roughness_texture.texcoord,
                .tex_type = RPE_MATERIAL_IMAGE_TYPE_METALLIC_ROUGHNESS};
            DYN_ARRAY_APPEND(&asset->textures, &mr_asset);
        }

        rpe_material_set_roughness_factor(new_mat, mat->pbr_metallic_roughness.roughness_factor);
        rpe_material_set_metallic_factor(new_mat, mat->pbr_metallic_roughness.metallic_factor);

        cgltf_float* bcf = mat->pbr_metallic_roughness.base_color_factor;
        math_vec4f vec = {bcf[0], bcf[1], bcf[2], bcf[3]};
        rpe_material_set_base_colour_factor(new_mat, &vec);
    }

    // normal texture
    if (mat->normal_texture.texture)
    {
        asset_texture_t normal_asset = {
            .mat = new_mat,
            .gltf_tex = &mat->normal_texture,
            .uv_index = mat->normal_texture.texcoord,
            .tex_type = RPE_MATERIAL_IMAGE_TYPE_NORMAL};
        DYN_ARRAY_APPEND(&asset->textures, &normal_asset);
    }
    // Occlusion texture.
    if (mat->occlusion_texture.texture)
    {
        asset_texture_t occlusion_asset = {
            .mat = new_mat,
            .gltf_tex = &mat->occlusion_texture,
            .uv_index = mat->occlusion_texture.texcoord,
            .tex_type = RPE_MATERIAL_IMAGE_TYPE_OCCLUSION};
        DYN_ARRAY_APPEND(&asset->textures, &occlusion_asset);
    }
    // Emissive texture.
    if (mat->emissive_texture.texture)
    {
        asset_texture_t emissive_asset = {
            .mat = new_mat,
            .gltf_tex = &mat->emissive_texture,
            .uv_index = mat->emissive_texture.texcoord,
            .tex_type = RPE_MATERIAL_IMAGE_TYPE_EMISSIVE};
        DYN_ARRAY_APPEND(&asset->textures, &emissive_asset);
    }
    // Parse the emissive factor and strength.
    math_vec4f ef = {
        mat->emissive_factor[0], mat->emissive_factor[1], mat->emissive_factor[2], 1.0f};
    if (mat->has_emissive_strength)
    {
        ef = math_vec4f_mul_sca(ef, mat->emissive_strength.emissive_strength);
    }
    rpe_material_set_emissive_factor(new_mat, &ef);

    // Specular - extension.
    if (mat->has_specular)
    {
        cgltf_float* sf = mat->specular.specular_color_factor;
        math_vec4f vec = {sf[0], sf[1], sf[2], 1.0f};
        rpe_material_set_specular_factor(new_mat, &vec);
    }

    // Alpha blending.
    rpe_material_set_alpha_cutoff(new_mat, mat->alpha_cutoff);
    rpe_material_set_alpha_mask(new_mat, gltf_model_material_convert_to_alpha(mat->alpha_mode));

    // Determines the type of culling required.
    rpe_material_set_double_sided_state(new_mat, mat->double_sided);

    if (mat->double_sided)
    {
        rpe_material_set_cull_mode(new_mat, RPE_CULL_MODE_NONE);
    }

    return new_mat;
}

bool create_mesh_instance(cgltf_mesh* mesh, gltf_asset_t* asset, rpe_object_t* transform_obj, arena_t* arena)
{
    assert(mesh);

    for (cgltf_size prim_idx = 0; prim_idx < mesh->primitives_count; ++prim_idx)
    {
        cgltf_primitive* primitive = &mesh->primitives[prim_idx];
        if (primitive->type != cgltf_primitive_type_triangles)
        {
            log_error("At the moment only triangles are supported by the "
                      "gltf parser.");
            return false;
        }

        // Only one material per mesh is allowed which is the case 99% of the
        // time cache the cgltf pointer and use to ensure this is the case.
        rpe_material_t* mesh_mat = create_material_instance(primitive->material, asset);
        DYN_ARRAY_APPEND(&asset->materials, &mesh_mat);

        // Get the number of vertices to process.
        assert(primitive->attributes[0].data);
        size_t vert_count = primitive->attributes[0].data->count;

        // ================ vertices =====================
        // Somewhere to store the base pointers and stride for each attribute.
        uint8_t* pos_base = NULL;
        uint8_t* norm_base = NULL;
        uint8_t* tangent_base = NULL;
        uint8_t* uv_base[RPE_RENDERABLE_MAX_UV_SET_COUNT] = {NULL};
        uint8_t* col_base = NULL;
        uint8_t* weights_base = NULL;
        uint8_t* joints_base = NULL;

        math_vec3f min, max;

        size_t pos_stride, norm_stride, uv_stride, weights_stride, joints_stride, col_stride,
            tangent_stride;
        size_t attrib_count = primitive->attributes_count;

        for (cgltf_size attrib_idx = 0; attrib_idx < attrib_count; ++attrib_idx)
        {
            cgltf_attribute* attrib = &primitive->attributes[attrib_idx];
            size_t index = attrib->index;

            if (attrib->type == cgltf_attribute_type_position)
            {
                pos_base = gltf_model_get_attr_data(attrib, &pos_stride);
                assert(pos_stride == 12);

                // The spec states that the position attribute must contain min/max values.
                float* minp = &attrib->data->min[0];
                float* maxp = &attrib->data->max[0];
                min.x = minp[0];
                min.y = minp[1];
                min.z = minp[2];
                max.x = maxp[0];
                max.y = maxp[1];
                max.z = maxp[2];
            }
            else if (attrib->type == cgltf_attribute_type_normal)
            {
                norm_base = gltf_model_get_attr_data(attrib, &norm_stride);
                assert(norm_stride == 12);
            }
            else if (attrib->type == cgltf_attribute_type_tangent)
            {
                tangent_base = gltf_model_get_attr_data(attrib, &tangent_stride);
                assert(tangent_stride == 16);
            }
            else if (attrib->type == cgltf_attribute_type_texcoord)
            {
                if (index >= RPE_RENDERABLE_MAX_UV_SET_COUNT)
                {
                    log_error("RPE only supports two uv sets.");
                    return false;
                }
                uv_base[index] = gltf_model_get_attr_data(attrib, &uv_stride);
                assert(uv_stride == 8);
            }
            else if (attrib->type == cgltf_attribute_type_color)
            {
                col_base = gltf_model_get_attr_data(attrib, &col_stride);
                assert(col_stride == 16);
            }
            else if (attrib->type == cgltf_attribute_type_joints)
            {
                if (index > 0)
                {
                    log_error("Only one set supported for joints.");
                    return false;
                }
                joints_base = gltf_model_get_attr_data(attrib, &joints_stride);
                assert(col_stride == 16);
            }
            else if (attrib->type == cgltf_attribute_type_weights)
            {
                if (index > 0)
                {
                    log_error("Only one set supported for bone weights.");
                    return false;
                }
                weights_base = gltf_model_get_attr_data(attrib, &weights_stride);
                assert(col_stride == 16);
            }
            else
            {
                log_warn(
                    "Gltf attribute not supported - %s; Attribute will be ignored.", attrib->name);
            }
        }

        // Must have position data otherwise we can't continue.
        if (!pos_base)
        {
            log_error("Gltf file contains no vertex position data. Unable "
                      "to continue.");
            return false;
        }

        // ================= indices ===================

        uint8_t* indices_base;
        enum IndicesType indices_type;
        size_t indices_count;

        // If the model doesn't contain indices, generate sequential index values.
        if (!primitive->indices || primitive->indices->count == 0)
        {
            uint32_t* indices = ARENA_MAKE_ZERO_ARRAY(arena, uint32_t, vert_count);
            for (size_t i = 0; i < vert_count; ++i)
            {
                indices[i] = (uint32_t)i;
            }
            indices_base = (uint8_t*)indices;
            indices_type = RPE_RENDERABLE_INDICES_U32;
            indices_count = vert_count;
        }
        else
        {
            indices_base = (uint8_t*)primitive->indices->buffer_view->buffer->data +
                primitive->indices->offset + primitive->indices->buffer_view->offset;
            indices_type = primitive->indices->component_type == cgltf_component_type_r_32u
                ? RPE_RENDERABLE_INDICES_U32
                : RPE_RENDERABLE_INDICES_U16;
            indices_count = primitive->indices->count;
        }

        rpe_valloc_handle v_handle = rpe_rend_manager_alloc_vertex_buffer(asset->rend_manager, vert_count);
        rpe_valloc_handle i_handle = rpe_rend_manager_alloc_index_buffer(asset->rend_manager, indices_count);
        rpe_mesh_t* new_mesh = rpe_rend_manager_create_mesh_interleaved(
            asset->rend_manager,
            v_handle,
            (float*)pos_base,
            (float*)uv_base[0],
            (float*)uv_base[1],
            (float*)norm_base,
            (float*)tangent_base,
            (float*)col_base,
            (float*)weights_base,
            (float*)joints_base,
            vert_count,
            i_handle,
            indices_base,
            indices_count,
            indices_type);
        DYN_ARRAY_APPEND(&asset->meshes, &new_mesh);

        rpe_renderable_t* renderable =
            rpe_engine_create_renderable(asset->engine, mesh_mat, new_mesh);

        rpe_renderable_set_min_max_dimensions(renderable, min, max);
        asset->aabbox.min = math_vec3f_min(asset->aabbox.min, min);
        asset->aabbox.max = math_vec3f_max(asset->aabbox.max, max);

        // Add the renderable to the manager all with the same transform.
        rpe_object_t mesh_obj =
            rpe_obj_manager_create_obj(rpe_engine_get_obj_manager(asset->engine));
        rpe_rend_manager_add(asset->rend_manager, renderable, mesh_obj, *transform_obj);
        DYN_ARRAY_APPEND(&asset->objects, &mesh_obj);
    }
    return true;
}

cgltf_node* find_node_recursive(const char* id, cgltf_node* node) // NOLINT
{
    cgltf_node* result = NULL;
    if (strcmp(node->name, id) == 0)
    {
        return node;
    }
    for (cgltf_size child_idx = 0; child_idx < node->children_count; ++child_idx)
    {
        result = find_node_recursive(id, node->children[child_idx]);
        if (result)
        {
            break;
        }
    }
    return result;
}

cgltf_node* gltf_node_get_node(cgltf_node* node, const char* id)
{
    assert(node);
    return find_node_recursive(id, node);
}

math_mat4f gltf_node_prepare_translation(cgltf_node* node)
{
    math_mat4f out = math_mat4f_identity();

    // Usually the gltf file will have a baked matrix or TRS data.
    if (node->has_matrix)
    {
        memcpy(out.data, node->matrix, sizeof(float) * 16);
    }
    else
    {
        math_vec3f translation = {0.0f, 0.0f, 0.0f};
        math_vec3f scale = {1.0f, 1.0f, 1.0f};
        math_quatf rot = {0.0f, 0.0f, 0.0f, 1.0f};

        if (node->has_translation)
        {
            translation.x = node->translation[0];
            translation.y = node->translation[1];
            translation.z = node->translation[2];
        }
        if (node->has_rotation)
        {
            rot.x = node->rotation[0];
            rot.y = node->rotation[1];
            rot.z = node->rotation[2];
            rot.w = node->rotation[3];
        }
        if (node->has_scale)
        {
            scale.x = node->scale[0];
            scale.y = node->scale[1];
            scale.z = node->scale[2];
        }

        math_mat4f T = math_mat4f_identity();
        math_mat4f S = math_mat4f_identity();
        math_mat4f R = math_quatf_to_mat4f(rot);
        math_mat4f_translate(translation, &T);
        math_mat4f_scale(scale, &S);
        out = math_mat4f_mul(T, math_mat4f_mul(R, S));
    }
    return out;
}

bool gltf_node_create_node_hierachy_recursive( // NOLINT
    cgltf_node* node,
    rpe_engine_t* engine,
    gltf_asset_t* asset,
    rpe_object_t* parent_obj,
    arena_t* arena)
{
    assert(node);
    assert(engine);

    rpe_transform_manager_t* tm = rpe_engine_get_transform_manager(engine);
    rpe_obj_manager_t* om = rpe_engine_get_obj_manager(engine);

    math_mat4f local_transform = gltf_node_prepare_translation(node);
    rpe_object_t obj = rpe_obj_manager_create_obj(om);
    rpe_object_t* obj_p = DYN_ARRAY_APPEND(&asset->objects, &obj);
    rpe_transform_manager_add_node(tm, &local_transform, parent_obj, obj_p);

    if (node->mesh)
    {
        if (!create_mesh_instance(node->mesh, asset, obj_p, arena))
        {
            return false;
        }
    }

    // now for the children of this node
    for (cgltf_size child_idx = 0; child_idx < node->children_count; ++child_idx)
    {
        if (!gltf_node_create_node_hierachy_recursive(
                node->children[child_idx], engine, asset, obj_p, arena))
        {
            return false;
        }
    }

    return true;
}

bool gltf_node_create_node_hierachy(cgltf_node* node, gltf_asset_t* asset, rpe_engine_t* engine, arena_t* arena)
{
    rpe_transform_manager_t* tm = rpe_engine_get_transform_manager(engine);

    rpe_object_t obj = rpe_obj_manager_create_obj(rpe_engine_get_obj_manager(engine));
    rpe_object_t* obj_p = DYN_ARRAY_APPEND(&asset->objects, &obj);
    math_mat4f t = math_mat4f_identity();
    rpe_transform_manager_add_node(tm, &t, NULL, &obj);

    if (!gltf_node_create_node_hierachy_recursive(node, engine, asset, obj_p, arena))
    {
        return false;
    }

    return true;
}

void linearise_nodes_recursive(gltf_asset_t* asset, cgltf_node* node, size_t index) // NOLINT
{
    // For most nodes, they don't expose a name, so we can't rely on this
    // for identifying nodes. So. we will use a stringifyed id instead
    // char temp[20];
    // sprintf(temp, "%lu", index++);
    // strcpy(node->name, temp);

    DYN_ARRAY_APPEND(&asset->nodes, node);

    for (cgltf_size child_idx = 0; child_idx < node->children_count; ++child_idx)
    {
        linearise_nodes_recursive(asset, node->children[child_idx], index);
    }
}

void linearise_nodes(cgltf_data* data, gltf_asset_t* asset)
{
    size_t index = 0;

    for (cgltf_size scene_idx = 0; scene_idx < data->scenes_count; ++scene_idx)
    {
        cgltf_scene* scene = &data->scenes[scene_idx];
        for (cgltf_size node_idx = 0; node_idx < scene->nodes_count; ++node_idx)
        {
            linearise_nodes_recursive(asset, scene->nodes[node_idx], index);
        }
    }
}

bool create_model_instance(gltf_asset_t* asset, arena_t* arena)
{
    assert(asset);
    cgltf_data* model_data = asset->model_data;

    // Joints and animation samplers point at nodes in the hierarchy. To link our
    // node hierarchy, the model nodes have their ids. We also linearise the cgltf
    // nodes in a map, along with an id which matches the pattern of the model
    // nodes. To find a model node from a cgltf node - find the id of the cgltf
    // in the linear map, and then search for this id in the model node hierarchy
    // and return the pointer.
    linearise_nodes(model_data, asset);

    // Prepare any extensions that may be included with this model.
    // struct GltfExtensions* extensions =
    //    gltf_extension_build(&model_data->extras, model_data, &asset->arena);

    // for each scene, visit each node in that scene
    for (cgltf_size scene_idx = 0; scene_idx < asset->model_data->scenes_count; ++scene_idx)
    {
        cgltf_scene* scene = &asset->model_data->scenes[scene_idx];
        for (cgltf_size node_idx = 0; node_idx < scene->nodes_count; ++node_idx)
        {
            if (!gltf_node_create_node_hierachy(scene->nodes[node_idx], asset, asset->engine, arena))
            {
                return false;
            }
        }
    }

    return true;
}

gltf_asset_t* create_asset(rpe_engine_t* engine, cgltf_data* model_data, const char* path, arena_t* arena)
{
    gltf_asset_t* new_asset = malloc(sizeof(gltf_asset_t));
    assert(new_asset);
    memset(new_asset, 0, sizeof(gltf_asset_t));

    new_asset->engine = engine;
    new_asset->model_data = model_data;
    new_asset->rend_manager = rpe_engine_get_rend_manager(engine);

    MAKE_DYN_ARRAY(rpe_object_t, arena, 30, &new_asset->objects);
    MAKE_DYN_ARRAY(asset_texture_t, arena, 30, &new_asset->textures);
    MAKE_DYN_ARRAY(cgltf_node, arena, 30, &new_asset->nodes);
    MAKE_DYN_ARRAY(rpe_material_t*, arena, 30, &new_asset->materials);
    MAKE_DYN_ARRAY(rpe_mesh_t*, arena, 30, &new_asset->meshes);
    new_asset->gltf_path = string_init(path, arena);

    return new_asset;
}

gltf_asset_t*
gltf_model_parse_data(uint8_t* gltf_data, size_t data_size, rpe_engine_t* engine, const char* path, arena_t* arena)
{
    // No additional options required.
    cgltf_options options = {0};

    cgltf_data* gltf_root;
    cgltf_result res = cgltf_parse(&options, gltf_data, data_size, &gltf_root);
    if (res != cgltf_result_success)
    {
        log_error("Error whilst parsing gltf data. Error code: %d\n", res);
        return NULL;
    }
    if (cgltf_validate(gltf_root) != cgltf_result_success)
    {
        log_error("The gltf data is invalid.");
        return NULL;
    }

    // The buffers need parsing separately.
    res = cgltf_load_buffers(&options, gltf_root, path);
    if (res != cgltf_result_success)
    {
        log_error("Unable to load gltf buffers.");
        return NULL;
    }

    // Create the GLTF asset which contains all the parsed information which can be used by the
    // client.
    gltf_asset_t* asset = create_asset(engine, gltf_root, path, arena);

    if (!create_model_instance(asset, arena))
    {
        return NULL;
    }

    return asset;
}

void gltf_model_create_instances(
    gltf_asset_t* assets,
    rpe_rend_manager_t* rm,
    rpe_transform_manager_t* tm,
    rpe_obj_manager_t* om,
    rpe_scene_t* scene,
    uint32_t count,
    rpe_model_transform_t* transforms,
    arena_t* arena)
{
    arena_dyn_array_t objects;
    MAKE_DYN_ARRAY(rpe_object_t, arena, 50, &objects);

    for (uint32_t i = 0; i < count; ++i)
    {
        for (size_t j = 0; j < assets->objects.size; ++j)
        {
            rpe_object_t model_obj = rpe_obj_manager_create_obj(om);

            rpe_object_t src_obj = DYN_ARRAY_GET(rpe_object_t, &assets->objects, j);
            if (rpe_rend_manager_has_obj(rm, &src_obj))
            {
                rpe_object_t rend_trans_obj = rpe_rend_manager_get_transform(rm, src_obj);
                rpe_object_t* parent_trans_obj =
                    rpe_transform_manager_get_parent(tm, rend_trans_obj);
                assert(parent_trans_obj);

                rpe_object_t model_trans_obj =
                    rpe_transform_manager_copy(tm, om, parent_trans_obj, &objects);
                rpe_transform_manager_set_transform(tm, model_trans_obj, &transforms[i]);

                rpe_rend_manager_copy(rm, tm, src_obj, model_obj, rpe_transform_manager_get_child(tm, model_trans_obj));
                // Add object to the scene.
                rpe_scene_add_object(scene, model_obj);
            }
        }
    }
}