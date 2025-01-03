#version 460

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColour;
layout(location = 4) in vec4 inWeights;
layout(location = 5) in vec4 inBoneId;
layout(location = 6) in uint inModelDrawIdx;
layout(location = 7) in uint inModelObjectId;

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outColour;
layout(location = 3) out uint outModelDrawIdx;
layout(location = 4) out vec3 outPos;

layout (constant_id = 0) const int HAS_SKIN = 0;
layout (constant_id = 1) const int HAS_NORMAL = 0;

#define MAX_BONES 250

layout (set = 2, binding = 0) buffer SkinSsbo
{
    mat4 bones[];
} skin_ssbo;

layout (set = 2, binding = 1) buffer TransformSSbo
{
    mat4 modelTransform[];
} transform_ssbo;

layout (binding = 0, set = 0) uniform CameraUbo
{
    mat4 mvp;
} camera_ubo;


void main()
{
    mat4 normalTransform;

    if (HAS_SKIN == 1)
    {
        mat4 boneTransform = skin_ssbo.bones[int(inBoneId.x)] * inWeights.x;
        boneTransform += skin_ssbo.bones[int(inBoneId.y)] * inWeights.y;
        boneTransform += skin_ssbo.bones[int(inBoneId.z)] * inWeights.z;
        boneTransform += skin_ssbo.bones[int(inBoneId.w)] * inWeights.w;
    
        normalTransform = transform_ssbo.modelTransform[inModelObjectId] * boneTransform;
    }
    else
    {
        normalTransform = transform_ssbo.modelTransform[inModelObjectId];
    }

    vec4 pos = normalTransform * vec4(inPos, 1.0);

    // inverse-transpose for non-uniform scaling - expensive computations here -
    // maybe remove this?
    if (HAS_NORMAL == 1)
    {
        outNormal = normalize(transpose(inverse(mat3(normalTransform))) * inNormal);
    }
    else
    {
        outNormal = vec3(0.0f, 0.0f, 0.0f);
    }

    outUv = inUv;
    outColour = inColour;

    outPos = pos.xyz / pos.w;
    gl_Position = camera_ubo.mvp * vec4(outPos, 1.0);
}
