#version 460

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outFrag;

layout (set = 3, binding = 0) uniform sampler2DArray cascadeMap;
layout (set = 3, binding = 1) uniform sampler2D colourSampler;

layout(push_constant) uniform Constants 
{
	layout(offset = 0) uint cascadeIdx;
} push;


void main()
{
    float depth = texture(cascadeMap, vec3(inUv, float(push.cascadeIdx))).r;
    if (depth == 1.0)
    {
        outFrag = texture(colourSampler, inUv);
    }
    else
    {
	    outFrag = vec4(vec3(depth), 1.0);
    }
}