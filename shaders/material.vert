#version 460

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

layout(location = 0) out vec2 outUv0;
layout(location = 1) out vec2 outUv1;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec4 outTangent;
layout(location = 4) out vec4 outColour;
layout(location = 5) out uint outModelDrawIdx;
layout(location = 6) out vec3 outPos;

layout (constant_id = 0) const bool HAS_SKIN = false;
layout (constant_id = 1) const bool HAS_NORMAL = false;

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
    mat4 modelTransform;

    if (HAS_SKIN)
    {
        mat4 boneTransform = skin_ssbo.bones[int(inBoneId.x)] * inWeights.x;
        boneTransform += skin_ssbo.bones[int(inBoneId.y)] * inWeights.y;
        boneTransform += skin_ssbo.bones[int(inBoneId.z)] * inWeights.z;
        boneTransform += skin_ssbo.bones[int(inBoneId.w)] * inWeights.w;
    
        modelTransform = transform_ssbo.modelTransform[inModelObjectId] * boneTransform;
    }
    else
    {
        modelTransform = transform_ssbo.modelTransform[inModelObjectId];
    }

    vec4 pos = modelTransform * vec4(inPos, 1.0);

    mat4 normalTransform = transpose(inverse(modelTransform));

    outNormal = HAS_NORMAL ? normalize(normalTransform * vec4(inNormal, 0.0)).rgb : vec3(0.0);
    outTangent = modelTransform * inTangent;

    outUv0 = inUv0;
    outUv1 = inUv1;
    outColour = inColour;
    outModelDrawIdx = inModelDrawIdx;

    outPos = pos.xyz /pos.w;
    gl_Position = camera_ubo.mvp * vec4(outPos, 1.0);
}
