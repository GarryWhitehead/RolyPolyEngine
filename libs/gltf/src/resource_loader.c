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
#include <rpe/engine.h>
#include <rpe/material.h>
#include <string.h>
#include <utility/filesystem.h>

gltf_resource_loader_t
gltf_resource_loader_init(rpe_engine_t* engine, cgltf_data* data, arena_t* arena)
{
    gltf_resource_loader_t rl = {0};
    rl.texture_cache = gltf_material_cache_init(data);
    MAKE_DYN_ARRAY(struct DecodeEntry, arena, 100, &rl.decode_queue);
    rl.parent_job = job_queue_create_job(rpe_engine_get_job_queue(engine), NULL, NULL, NULL);
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

sampler_params_t gltf_resource_loader_create_sampler(gltf_asset_t* asset, cgltf_sampler* sampler)
{
    assert(asset);

    sampler_params_t out = {0};

    // Check whether this texture has a sampler.
    if (sampler)
    {
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

    return out;
}

bool decode_image(gltf_resource_loader_t* rl, gltf_asset_t* asset, struct DecodeEntry* entry, struct Job* parent)
{
    bool res = false;
    if (strcmp("image/png", entry->mime_type.data) == 0 ||
        strcmp("image/jpeg", entry->mime_type.data) == 0 ||
        strcmp("image/jpg", entry->mime_type.data) == 0)
    {
        gltf_stb_loader_push_job(asset->engine, entry, parent);
    }
    else if (strcmp("image/ktx2", entry->mime_type.data) == 0)
    {
        gltf_ktx_loader_push_job(asset->engine, entry, parent);
    }
    else
    {
        log_error("Unsupported image mime type: %s; Unable to load image", entry->mime_type.data);
        res = false;
    }
    return res;
}

gltf_image_handle_t get_texture(
    gltf_resource_loader_t* rl,
    cgltf_texture_view* view,
    gltf_asset_t* asset,
    image_free_func* free_func,
    arena_t* arena)
{
    cgltf_texture* texture = view->texture;

    gltf_image_handle_t out_handle = gltf_material_cache_get_entry(&rl->texture_cache, texture);
    if (out_handle.id != UINT32_MAX)
    {
        return out_handle;
    }

    string_t mime_type = texture->image->mime_type
        ? string_init(texture->image->mime_type, arena)
        : string_init("", arena);
    size_t uri_data_bytes_count;
    uint8_t* uri_data_bytes = texture->image->uri
        ? parse_data_uri(texture->image->uri, &mime_type, &uri_data_bytes_count, arena)
        : NULL;

    if (uri_data_bytes)
    {
        out_handle = gltf_material_cache_push_pending(&rl->texture_cache, texture);
        rpe_mapped_texture_t* t = &gltf_material_cache_get(&rl->texture_cache, out_handle)->texture;

        struct DecodeEntry entry = {
            .image_data = uri_data_bytes,
            .image_sz = uri_data_bytes_count,
            .mapped_texture = t,
            .free_func = free_func,
            .mime_type = mime_type};
        DYN_ARRAY_APPEND(&rl->decode_queue, &entry);
    }

    // If the filesystem uri is defined, then load from disk.
    else if (texture->image->uri)
    {
        // Not cached, so try and load from disk.
        string_t path = fs_remove_filename(&asset->gltf_path, arena);
        string_t full_path = string_append3(&path, "/", texture->image->uri, arena);
        FILE* fp = fopen(full_path.data, "r");
        if (!fp)
        {
            log_error("Unable to open image at uri: %s", full_path);
            return out_handle;
        }
        size_t sz = fs_get_file_size(fp);
        void* image_data = ARENA_MAKE_ZERO_ARRAY(arena, uint8_t, sz);
        size_t sz_read = fread(image_data, sizeof(uint8_t), sz, fp);
        if (sz_read != sz)
        {
            log_error("Error whilst reading image file: %s", full_path);
            return out_handle;
        }
        fclose(fp);

        if (strcmp(mime_type.data, "") == 0)
        {
            string_t suffix;
            string_t filename = {.data = texture->image->uri, strlen(texture->image->uri)};
            bool r = fs_get_extension(&filename, &suffix, arena);
            assert(r);
            string_t parent = string_init("image/", arena);
            mime_type = string_append(&parent, suffix.data, arena);
        }

        out_handle = gltf_material_cache_push_pending(&rl->texture_cache, texture);
        rpe_mapped_texture_t* t = &gltf_material_cache_get(&rl->texture_cache, out_handle)->texture;

        struct DecodeEntry entry = {
            .image_data = image_data,
            .image_sz = sz,
            .mapped_texture = t,
            .free_func = free_func,
            .mime_type = mime_type};
        DYN_ARRAY_APPEND(&rl->decode_queue, &entry);
    }
    else if (texture->image->buffer_view)
    {
        void* bvd = texture->image->buffer_view->data ? texture->image->buffer_view->data
                                                      : texture->image->buffer_view->buffer->data;
        assert(bvd);
        uint8_t* source_ptr = texture->image->buffer_view->offset + (uint8_t*)bvd;

        out_handle = gltf_material_cache_push_pending(&rl->texture_cache, texture);
        rpe_mapped_texture_t* t = &gltf_material_cache_get(&rl->texture_cache, out_handle)->texture;

        struct DecodeEntry entry = {
            .image_data = source_ptr,
            .image_sz = texture->image->buffer_view->size,
            .mapped_texture = t,
            .free_func = free_func,
            .mime_type = mime_type};
        DYN_ARRAY_APPEND(&rl->decode_queue, &entry);
    }
    else
    {
        log_error("Unable to create texture for: %s", view->texture->name);
    }

    return out_handle;
}

void gltf_resource_loader_load_textures(gltf_asset_t* asset, rpe_engine_t* engine, arena_t* arena)
{
    gltf_resource_loader_t rl = gltf_resource_loader_init(engine, asset->model_data, arena);

    // Generate the required image decoding work for materials.
    for (size_t i = 0; i < asset->textures.size; ++i)
    {
        struct AssetTextureParams* params =
            DYN_ARRAY_GET_PTR(struct AssetTextureParams, &asset->textures, i);
        params->mat_texture = get_texture(&rl, params->gltf_tex, asset, &params->free_func, arena);
    }

    // Decode the images.
    for (size_t i = 0; i < rl.decode_queue.size; ++i)
    {
        struct DecodeEntry* entry = DYN_ARRAY_GET_PTR(struct DecodeEntry, &rl.decode_queue, i);
        decode_image(&rl, asset, entry, rl.parent_job);
    }

    for (size_t i = 0; i < rl.decode_queue.size; ++i)
    {
        struct DecodeEntry* entry = DYN_ARRAY_GET_PTR(struct DecodeEntry, &rl.decode_queue, i);
        job_queue_wait_and_release(rpe_engine_get_job_queue(engine), entry->decoder_job);
    }

    // Upload the decoded images to the device.
    for (uint32_t i = 0; i < rl.texture_cache.count; ++i)
    {
        struct ImageEntry* entry = &rl.texture_cache.entries[i];

        sampler_params_t sampler = gltf_resource_loader_create_sampler(asset, entry->sampler);
        bool gen_mipmaps = !entry->texture.mip_levels ? true : false;
        entry->backend_handle =
            rpe_material_map_texture(engine, &entry->texture, &sampler, gen_mipmaps);
    }

    // Create the material from the generated images and params.
    for (size_t i = 0; i < asset->textures.size; ++i)
    {
        struct AssetTextureParams* params =
            DYN_ARRAY_GET_PTR(struct AssetTextureParams, &asset->textures, i);

        struct ImageEntry* entry = gltf_material_cache_get(&rl.texture_cache, params->mat_texture);
        rpe_material_set_device_texture(
            params->mat, entry->backend_handle, params->tex_type, params->uv_index);

        // Done with the image buffers so delete.
        if (params->free_func)
        {
            params->free_func(&entry->texture.image_data);
        }
    }
}
