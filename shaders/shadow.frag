#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "include/draw_data.h"

layout (location = 0) in vec2 inUv0;
layout (location = 1) in vec2 inUv1;
layout (location = 2) in vec4 inColour;
layout (location = 3) in flat uint inModelDrawIdx;

layout(set = 3, binding = 0) uniform sampler2D textures[];

layout (set = 2, binding = 2) buffer MeshDataSsbo
{
    DrawData drawData[];
};

void main()
{
    DrawData iData = drawData[inModelDrawIdx];
    
    vec2 uv = iData.colourUv == 0 ? inUv0 : inUv1;
    vec4 baseColour = texture(textures[nonuniformEXT(iData.colour)], uv);

    if (baseColour.a < 0.5)
    {
        discard;
    }
}