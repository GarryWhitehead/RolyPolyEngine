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
#include "ibl_helper.h"

#include <ktx.h>
#include <rpe/engine.h>
#include <rpe/ibl.h>
#include <stb_image.h>

bool ibl_load_eqirect_hdr_image(ibl_t* ibl, rpe_engine_t* engine, const char* path)
{
    rpe_ibl_envmap_info_t info = {0};

    int width, height, comp;
    float* data = stbi_loadf(path, &width, &height, &comp, 3);
    if (!data)
    {
        log_error("Unable to load image at path %s.", path);
        return false;
    }

    if (!stbi_is_hdr(path))
    {
        log_error("Image must be in the hdr format for ibl env map.");
        return false;
    }

    info.data_sz = width * height * 4;
    info.data = malloc(info.data_sz * sizeof(float));
    info.width = width;
    info.height = height;
    info.mips = 0;

    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            uint32_t old_pixel_idx = (i * width + j) * 3;
            uint32_t new_pixel_idx = (i * width + j) * 4;

            float new_pixel[4] = {
                data[old_pixel_idx], data[old_pixel_idx + 1], data[old_pixel_idx + 2], 1.0f};
            memcpy(info.data + new_pixel_idx, new_pixel, sizeof(float) * 4);
        }
    }
    stbi_image_free(data);

    rpe_ibl_eqirect_to_cubemap(ibl, engine, info.data, info.width, info.height);

    free(info.data);
    return true;
}

bool ibl_load_cubemap_ktx(ibl_t* ibl, rpe_engine_t* engine, const char* path)
{
    rpe_ibl_envmap_info_t info = {0};

    ktxTexture* texture;
    KTX_error_code result =
        ktxTexture_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

    if (result != KTX_SUCCESS)
    {
        log_error("Unable to load ktx model file: %s", ktxErrorString(result));
        return false;
    }

    if (texture->numFaces != 6)
    {
        log_error("An environment model must be a cubemap.");
        return false;
    }

    info.width = texture->baseWidth;
    info.height = texture->baseHeight;
    info.mips = texture->numLevels;
    info.data = (float*)texture->pData;
    info.data_sz = texture->dataSize;

    size_t offsets[12 * 6];
    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t level = 0; level < info.mips; ++level)
        {
            KTX_error_code ret = ktxTexture_GetImageOffset(
                texture, level, 0, face, &offsets[face * info.mips + level]);
            assert(ret == KTX_SUCCESS);
        }
    }

    rpe_ibl_upload_cubemap(
        ibl, engine, info.data, info.data_sz, info.width, info.height, info.mips, offsets);

    ktxTexture_Destroy(texture);
    return true;
}