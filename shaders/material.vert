#version 460

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColour;
layout(location = 4) in vec4 inWeights;
layout(location = 5) in vec4 inBoneId;

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outColour;

// It is a fundamental requirement that positional data
// is passed to the fragment shader.
layout(location = 3) out vec3 outPos;

layout (constant_id = 0) const int HAS_SKIN = 0;

#define MAX_BONES 250

layout (set = 1, binding = 0) buffer SkinSsbo
{
    mat4 bones[];
} skin_ubo;

layout (set = 1, binding = 0) uniform SceneUbo
{
    mat4 model;
    mat4 mvp;
} scene_ubo;

void main()
{
    mat4 normalTransform;

    if (HAS_SKIN == 1)
    {
        mat4 boneTransform = skin_ubo.bones[int(inBoneId.x)] * inWeights.x;
        boneTransform += skin_ubo.bones[int(inBoneId.y)] * inWeights.y;
        boneTransform += skin_ubo.bones[int(inBoneId.z)] * inWeights.z;
        boneTransform += skin_ubo.bones[int(inBoneId.w)] * inWeights.w;
    
        normalTransform = scene_ubo.model * boneTransform;
    }
    else
    {
        normalTransform = scene_ubo.model;
    }

    vec4 pos = normalTransform * vec4(inPos, 1.0);

    // inverse-transpose for non-uniform scaling - expensive computations here -
    // maybe remove this?
    outNormal = normalize(transpose(inverse(mat3(normalTransform))) * inNormal);

    outUv = inUv;
    outColour = inColour;

    outPos = pos.xyz / pos.w;
    gl_Position = scene_ubo.mvp * vec4(outPos, 1.0);
}
