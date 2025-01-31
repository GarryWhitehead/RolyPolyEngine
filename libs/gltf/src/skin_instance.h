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

#ifndef __GLTF_SKIN_INSTANCE_H__
#define __GLTF_SKIN_INSTANCE_H__

#include <cgltf.h>
#include <utility/maths.h>
#include <utility/string.h>

typedef struct GltfNodeInfo gltf_node_info_t;
typedef struct GltfNodeInstance gltf_node_instance_t;

typedef struct GltfSkinInstance
{
    string_t name;

    // Links the bone node name with the inverse transform.
    math_mat4f* inv_bind_matrices;
    uint32_t inv_bind_matrix_count;

    // A list of joints - points to the node in the skeleton hierarchy which will
    // be transformed.
    gltf_node_info_t* joint_nodes;

    // A pointer to the root of the skeleton. The spec states that
    // the model doesn't need to specify this - thus will be NULL if this is the
    // case.
    gltf_node_info_t* skeleton_root;
} gltf_skin_instance_t;

bool gltf_skin_instance_parse(
    gltf_skin_instance_t* i, cgltf_skin* skin, gltf_node_instance_t* node, arena_t* arena);

#endif
