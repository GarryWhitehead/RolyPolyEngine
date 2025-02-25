#version 460

#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec4 outColour;

layout(set = 3, binding = 0) uniform sampler2D BaseColourSampler;

void main()
{
    vec3 v = normalize(inPos.xyz);

    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *=  vec2(0.1591, 0.3183);
    uv += 0.5;

    // write out color to output cubemap
    outColour = vec4(texture(BaseColourSampler, uv).rgb, 1.0);
}