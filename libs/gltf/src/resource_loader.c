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

#include "resource_loader.h"

#include "gltf/gltf_asset.h"
#include "gltf/resource_loader.h"
#include "ktx_loader.h"
#include "stb_loader.h"

#include <backend/enums.h>
#include <cgltf.h>
#include <log.h>
#include <rpe/material.h>
#include <string.h>
#include <utility/filesystem.h>
#include <utility/hash.h>

gltf_resource_loader_t gltf_resource_loader_init(arena_t* arena)
{
    gltf_resource_loader_t rl = {0};
    rl.uri_filename_cache = HASH_SET_CREATE(char*, rpe_mapped_texture_t, arena);
    rl.buffer_texture_cache = HASH_SET_CREATE(uint8_t*, rpe_mapped_texture_t, arena);
    return rl;
}

enum SamplerFilter gltf_resource_loader_get_sampler_filter(int filter)
{
    enum SamplerFilter result;
    switch (filter)
    {
        case 9728:
        case 9984:
        case 9985:
            result = RPE_SAMPLER_FILTER_NEAREST;
            break;
        case 9729:
        case 9986:
        case 9987:
            result = RPE_SAMPLER_FILTER_LINEAR;
            break;
        default:
            result = RPE_SAMPLER_FILTER_NEAREST;
    }
    return result;
}

enum SamplerAddressMode gltf_resource_loader_get_addr_mode(int mode)
{
    enum SamplerAddressMode result;

    switch (mode)
    {
        case 10497:
            result = RPE_SAMPLER_ADDR_MODE_REPEAT;
            break;
        case 33071:
            result = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE;
            break;
        case 33648:
            result = RPE_SAMPLER_ADDR_MODE_MIRRORED_REPEAT;
            break;
        default:
            result = RPE_SAMPLER_ADDR_MODE_REPEAT;
    }
    return result;
}

// Note: Adapted from meshoptimizer for C.
uint8_t* parse_data_uri(const char* uri, string_t* mime_type, size_t* sz, arena_t* arena)
{
    if (strncmp(uri, "data:", 5) != 0)
    {
        return NULL;
    }

    const char* comma = strchr(uri, ',');
    if (comma && comma - uri >= 7 && strncmp(comma - 7, ";base64", 7) == 0)
    {
        const char* base64 = comma + 1;
        size_t base64_size = strlen(base64);
        size_t size = base64_size - base64_size / 4;
        if (base64_size >= 2)
        {
            size -= base64[base64_size - 2] == '=';
            size -= base64[base64_size - 1] == '=';
        }

        void* data = NULL;
        cgltf_options options = {};
        cgltf_result res = cgltf_load_buffer_base64(&options, size, base64, &data);
        if (res != cgltf_result_success)
        {
            return NULL;
        }

        string_t uri_str = string_init(uri + 5, arena);
        *mime_type = string_append(&uri_str, comma - 7, arena);
        *sz = size;
        return (uint8_t*)data;
    }

    return NULL;
}

sampler_params_t gltf_resource_loader_create_sampler(gltf_asset_t* asset, cgltf_texture_view* view)
{
    assert(asset);
    assert(view);

    sampler_params_t out = {0};

    // Not guaranteed to have a texture or uri.
    if (view->texture->image)
    {
        // Check whether this texture has a sampler.
        if (view->texture->sampler)
        {
            cgltf_sampler* sampler = view->texture->sampler;
            out.mag = gltf_resource_loader_get_sampler_filter(sampler->mag_filter);
            out.min = gltf_resource_loader_get_sampler_filter(sampler->min_filter);
            out.addr_u = gltf_resource_loader_get_addr_mode(sampler->wrap_s);
            out.addr_v = gltf_resource_loader_get_addr_mode(sampler->wrap_t);
        }
        else
        {
            // Default sampler settings specified by the spec.
            out.mag = RPE_SAMPLER_FILTER_LINEAR;
            out.min = RPE_SAMPLER_FILTER_LINEAR;
            out.addr_u = RPE_SAMPLER_ADDR_MODE_REPEAT;
            out.addr_v = RPE_SAMPLER_ADDR_MODE_REPEAT;
        }
    }

    return out;
}

bool decode_image(
    string_t* mime_type,
    void* data,
    size_t sz,
    rpe_mapped_texture_t* tex,
    gltf_asset_t* asset,
    image_free_func* free_func)
{
    bool res;
    if (strcmp("image/png", mime_type->data) == 0 || strcmp("image/jpeg", mime_type->data) == 0 ||
        strcmp("image/jpg", mime_type->data) == 0)
    {
        res = gltf_stb_loader_decode_image(data, sz, tex, free_func);
    }
    else if (strcmp("image/ktx2", mime_type->data) == 0)
    {
        res = gltf_ktx_loader_decode_image(data, sz, tex, &asset->arena, free_func);
    }
    else
    {
        log_error("Unsupported image mime type: %s; Unable to load image", mime_type);
        res = false;
    }
    return res;
}

rpe_mapped_texture_t* get_texture(
    gltf_resource_loader_t* rl,
    cgltf_texture_view* view,
    gltf_asset_t* asset,
    image_free_func* free_func)
{
    cgltf_image* image = view->texture->image;
    rpe_mapped_texture_t new_tex = {0};
    rpe_mapped_texture_t* out_tex = NULL;

    string_t mime_type = image->mime_type ? string_init(image->mime_type, &asset->arena)
                                          : string_init("", &asset->arena);
    size_t uri_data_bytes_count;
    uint8_t* uri_data_bytes = image->uri
        ? parse_data_uri(image->uri, &mime_type, &uri_data_bytes_count, &asset->arena)
        : NULL;

    if (uri_data_bytes)
    {
        if (!decode_image(
                &mime_type, uri_data_bytes, uri_data_bytes_count, &new_tex, asset, free_func))
        {
            return NULL;
        }
        out_tex = HASH_SET_INSERT(&rl->buffer_texture_cache, &image->uri, &new_tex);
    }

    // If the filesystem uri is defined, then load from disk.
    else if (image->uri)
    {
        // First check whether the texture is already cached.
        rpe_mapped_texture_t* tex = HASH_SET_GET(&rl->uri_filename_cache, &image->uri);
        if (tex)
        {
            return tex;
        }

        // Not cached, so try and load from disk.
        string_t path = fs_remove_filename(&asset->gltf_path, &asset->arena);
        string_t full_path = string_append3(&path, "/", image->uri, &asset->arena);
        FILE* fp = fopen(full_path.data, "r");
        if (!fp)
        {
            log_error("Unable to open image at uri: %s", full_path);
            return NULL;
        }
        size_t sz = fs_get_file_size(fp);
        void* image_data = ARENA_MAKE_ZERO_ARRAY(&asset->arena, uint8_t, sz);
        size_t sz_read = fread(image_data, sizeof(uint8_t), sz, fp);
        if (sz_read != sz)
        {
            log_error("Error whilst reading image file: %s", full_path);
            return NULL;
        }
        fclose(fp);

        if (strcmp(mime_type.data, "") == 0)
        {
            string_t suffix;
            string_t filename = {.data = image->uri, strlen(image->uri)};
            bool r = fs_get_extension(&filename, &suffix, &asset->arena);
            assert(r);
            string_t parent = string_init("image/", &asset->arena);
            mime_type = string_append(&parent, suffix.data, &asset->arena);
        }

        if (!decode_image(&mime_type, image_data, sz, &new_tex, asset, free_func))
        {
            log_error("Error whilst decoding image: %s", image->uri);
            return NULL;
        }
        out_tex = HASH_SET_INSERT(&rl->uri_filename_cache, &image->uri, &new_tex);
    }
    else if (image->buffer_view)
    {
        // Check the data cache first, the texture may have already been stumbled upon in another
        // texture.
        void* bvd =
            image->buffer_view->data ? image->buffer_view->data : image->buffer_view->buffer->data;
        assert(bvd);
        uint8_t* source_ptr = image->buffer_view->offset + bvd;
        rpe_mapped_texture_t* tex = HASH_SET_GET(&rl->buffer_texture_cache, &source_ptr);
        if (tex)
        {
            return tex;
        }
        if (!decode_image(
                &mime_type, source_ptr, image->buffer_view->size, &new_tex, asset, free_func))
        {
            return NULL;
        }
        out_tex = HASH_SET_INSERT(&rl->buffer_texture_cache, &source_ptr, &new_tex);
    }
    else
    {
        log_error("Unable to create texture for: %s", view->texture->name);
        return NULL;
    }

    return out_tex;
}

void gltf_resource_loader_load_textures(gltf_asset_t* asset, rpe_engine_t* engine)
{
    gltf_resource_loader_t rl = gltf_resource_loader_init(&asset->arena);

    for (size_t i = 0; i < asset->textures.size; ++i)
    {
        struct AssetTextureParams* params =
            DYN_ARRAY_GET_PTR(struct AssetTextureParams, &asset->textures, i);
        rpe_mapped_texture_t* tex = get_texture(&rl, params->gltf_tex, asset, &params->free_func);
        if (tex)
        {
            params->mat_texture = *tex;
        }
    }

    // When async decoding is added, a wait will need to be called here to each loader to ensure all
    // images have been decoded before proceeding.

    for (size_t i = 0; i < asset->textures.size; ++i)
    {
        struct AssetTextureParams* params =
            DYN_ARRAY_GET_PTR(struct AssetTextureParams, &asset->textures, i);

        // Add texture to the image - this uploads the texture to the GPU.
        sampler_params_t sampler = gltf_resource_loader_create_sampler(asset, params->gltf_tex);
        bool gen_mipmaps = !params->mat_texture.mip_levels ? true : false;
        rpe_material_set_texture(
            params->mat,
            engine,
            &params->mat_texture,
            params->tex_type,
            &sampler,
            params->uv_index,
            gen_mipmaps);

        // Done with the image buffers so delete.
        if (params->free_func)
        {
            params->free_func(&params->mat_texture.image_data);
        }
    }
}
