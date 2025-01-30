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

#include "ktx_loader.h"

#include <ktx.h>
#include <log.h>
#include <rpe/material.h>

void gltf_ktx_loader_free_image(void* image) { ktxTexture_Destroy((ktxTexture*)image); }

bool gltf_ktx_loader_decode_image(
    void* data, size_t sz, rpe_mapped_texture_t* tex, arena_t* arena, image_free_func* free_func)
{
    assert(tex);

    ktxTexture* texture;
    KTX_error_code result =
        ktxTexture_CreateFromMemory(data, sz, KTX_TEXTURE_CREATE_NO_FLAGS, &texture);

    if (result != KTX_SUCCESS)
    {
        log_error("Unable to decode ktx image file: %s", ktxErrorString(result));
        return false;
    }

    assert(texture->numDimensions == 2 && "Only 2D textures supported by the engine currently.");

    tex->width = texture->baseWidth;
    tex->height = texture->baseHeight;
    tex->face_count = texture->numFaces;
    tex->mip_levels = texture->numLevels;
    tex->image_data = texture->pData;
    tex->image_data_size = texture->dataSize;
    tex->offsets = ARENA_MAKE_ZERO_ARRAY(arena, size_t, tex->face_count * tex->mip_levels);
    for (uint32_t face = 0; face < tex->face_count; face++)
    {
        for (uint32_t level = 0; level < tex->mip_levels; ++level)
        {
            KTX_error_code ret = ktxTexture_GetImageOffset(
                texture, level, 0, face, &tex->offsets[face * tex->mip_levels + level]);
            assert(ret == KTX_SUCCESS && "Error whilst generating image offsets.");
        }
    }

    *free_func = gltf_ktx_loader_free_image;

    return true;
}