/* Copyright (c) 2022 Garry Whitehead
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

#include "skin_instance.h"

#include <log.h>
#include <string.h>

/*bool gltf_skin_instance_parse(gltf_skin_instance_t* i, cgltf_skin* skin, gltf_node_instance_t*
node, arena_t* arena)
{
    // Extract the inverse bind matrices.
    const cgltf_accessor* accessor = skin->inverse_bind_matrices;
    uint8_t* base = (uint8_t*)accessor->buffer_view->buffer->data;

    // Use the stride as a sanity check to make sure we have a matrix.
    size_t stride = accessor->buffer_view->stride;
    if (!stride)
    {
        stride = accessor->stride;
    }
    assert(stride == 16);

    i->inv_bind_matrices = ARENA_MAKE_ZERO_ARRAY(arena, math_mat4f, accessor->count);
    i->inv_bind_matrix_count = accessor->count;
    memcpy(i->inv_bind_matrices, base, accessor->count * sizeof(math_mat4f));

    if (i->inv_bind_matrix_count != skin->joints_count)
    {
        log_error("The inverse bind matrices and joint sizes don't match.\n");
        return false;
    }

    // And now for bones...
    i->joint_nodes = ARENA_MAKE_ZERO_ARRAY(arena, gltf_node_info_t, i->inv_bind_matrix_count);
    for (size_t idx = 0; idx < skin->joints_count; ++idx)
    {
        cgltf_node* bone_node = skin->joints[idx];

        // find the node in the list
        gltf_node_info_t* found_node = gltf_node_get_node(node, bone_node->name);
        if (!found_node)
        {
            log_error("Unable to find bone in list of nodes with id: %s", bone_node->name);
            return false;
        }
        i->joint_nodes[idx] = *found_node;
    }

    // The model may not have a root for the skeleton. This isn't a requirement
    // by the spec.
    if (skin->skeleton)
    {
        i->skeleton_root = gltf_node_get_node(node, skin->skeleton->name);
    }

    return true;
}*/
