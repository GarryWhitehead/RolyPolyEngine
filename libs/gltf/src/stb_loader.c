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

#include "stb_loader.h"

#include <utility/string.h>

#include <stb_image.c>

void gltf_stb_loader_free_image(void* data) { stbi_image_free(data); }

bool gltf_stb_loader_decode_image(
    void* data, size_t sz, rpe_mapped_texture_t* tex, image_free_func* free_func)
{
    assert(tex);
    assert(data);

    int width, height, comp;
    if (!stbi_info_from_memory(data, (int)sz, &width, &height, &comp))
    {
        log_error("Unable to parse info from data buffer: %s", stbi_failure_reason());
        return false;
    }

    void* image_buffer = stbi_load_from_memory(data, (int)sz, &width, &height, &comp, 4);
    if (!image_buffer)
    {
        log_error("Unable to parse image: %s", stbi_failure_reason());
        return false;
    }

    tex->image_data = image_buffer;
    tex->image_data_size = width * height * 4;
    tex->width = width;
    tex->height = height;
    tex->face_count = 1;
    tex->format = VK_FORMAT_R8G8B8A8_UNORM;

    *free_func = NULL;

    return true;
}
