#ifndef MODEL_MESH_TYPES_H
#define MODEL_MESH_TYPES_H

#include "common.h"

void defaultMesh(vec4 pos)
{
    outPos = pos.xyz / pos.w;
    gl_Position = camera_ubo.mvp * vec4(outPos, 1.0);
}

void skyboxMesh(vec4 pos)
{
    // Remove translation part from matrix.
    mat4 viewMatrix = mat4(mat3(camera_ubo.view));
    gl_Position = camera_ubo.proj * viewMatrix * vec4(inPos.xyz, 1.0);
    gl_Position.y *= -1.0f;

    // Ensure skybox is renderered on the far plane.
    gl_Position.z = gl_Position.w;
    outPos = inPos;
}

void uiMesh(vec4 pos)
{
    gl_Position = camera_ubo.proj * vec4(pos.xy, 0.0, 1.0);
    outPos = vec3(pos.xy, 0.0);
}

void modelVertex(vec4 pos)
{
    switch (MATERIAL_TYPE)
    {
        case MATERIAL_TYPE_DEFAULT: {
            defaultMesh(pos);
            break;
        }
        case MATERIAL_TYPE_SKYBOX: {
            skyboxMesh(pos);
            break;
        }
        case MATERIAL_TYPE_UI: {
            uiMesh(pos);
            break;
        }
    }
}

#endif