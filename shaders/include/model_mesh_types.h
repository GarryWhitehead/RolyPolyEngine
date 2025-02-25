#ifndef MODEL_MESH_TYPES_H
#define MODEL_MESH_TYPES_H

#include "common.h"

void defaultMesh(vec4 pos)
{
    outPos = pos.xyz /pos.w;
    gl_Position = camera_ubo.mvp * vec4(outPos, 1.0);
}

void skyboxMesh(vec4 pos)
{
    // Remove translation part from matrix.
    mat4 viewMatrix = mat4(mat3(camera_ubo.view));
    gl_Position = camera_ubo.proj * viewMatrix * vec4(inPos.xyz, 1.0);

    // Ensure skybox is renderered on the far plane.
    gl_Position.z = gl_Position.w;

    outPos = inPos;
}

void modelVertex(vec4 pos)
{
    switch (MATERIAL_TYPE)
    {
        case MATERIAL_TYPE_DEFAULT:
        {
            defaultMesh(pos);
            break;
        }
        case MATERIAL_TYPE_SKYBOX:
        {
            skyboxMesh(pos);
            break;
        }
    }
}

#endif