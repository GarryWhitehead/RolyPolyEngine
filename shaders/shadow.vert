#version 460

#extension GL_EXT_multiview : enable
#extension GL_GOOGLE_include_directive : enable

#include "include/shadow.h"

// Mimic the model mesh vertices as they are already uploaded to the device.
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv0;
layout(location = 3) in vec2 inUv1;
layout(location = 4) in vec4 inTangent;
layout(location = 5) in vec4 inColour;
layout(location = 6) in vec4 inWeights;
layout(location = 7) in vec4 inBoneId;
layout(location = 8) in uint inModelDrawIdx;
layout(location = 9) in uint inModelObjectId;

layout (location = 0) out vec2 outUv0;
layout (location = 1) out vec2 outUv1;
layout (location = 2) out vec4 outColour;
layout (location = 3) out uint outModelDrawIdx;

layout (set = 2, binding = 0) buffer CascadeSSbo
{
    CascadeInfo cascades[];
};

layout (set = 2, binding = 1) buffer TransformSSbo
{
    mat4 modelTransform[];
};

void main()
{   
    gl_Position = cascades[gl_ViewIndex].vp * modelTransform[inModelObjectId] * vec4(inPos, 1.0);
    outUv0 = inUv0;
    outUv1 = inUv1;
    outColour = inColour;
    outModelDrawIdx = inModelDrawIdx;
}