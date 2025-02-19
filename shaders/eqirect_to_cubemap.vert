#version 460

#extension GL_EXT_multiview : enable

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec3 outPos;

layout (binding = 0, set = 0) uniform CameraUbo
{
    mat4 mvp;
    mat4 proj;
} camera_ubo;

layout (binding = 1, set = 0) uniform Ubo
{
    mat4 faceViews[6];
} ubo;

void main()
{
    outPos = inPos.xyz;
    //outPos.xy *= -1.0;
    
    gl_Position = camera_ubo.proj * ubo.faceViews[gl_ViewIndex] * vec4(inPos, 1.0);
}