#version 460

#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outFrag;

layout(set = 3, binding = 0) uniform sampler2D PositionSampler;
layout(set = 3, binding = 1) uniform sampler2D BaseColourSampler;
layout(set = 3, binding = 2) uniform sampler2D NormalSampler;
layout(set = 3, binding = 3) uniform sampler2D PbrSampler;
layout(set = 3, binding = 4) uniform sampler2D EmissiveSampler;
layout(set = 3, binding = 5) uniform sampler2D BrdfSampler;
layout(set = 3, binding = 6) uniform samplerCube IrradianceEnvMap;
layout(set = 3, binding = 7) uniform samplerCube SpecularEnvMap;
layout(set = 3, binding = 8) uniform sampler2DArray shadowMap;

layout (constant_id = 0) const bool HAS_IBL = false;
layout (constant_id = 1) const uint LIGHT_COUNT = 0;
layout (constant_id = 2) const uint SHADOW_CASCADE_COUNT = 1;
layout (constant_id = 3) const bool DRAW_SHADOWS = true;

#define LIGHT_TYPE_POINT        0
#define LIGHT_TYPE_SPOT         1   
#define LIGHT_TYPE_DIRECTIONAL  2

layout (binding = 0, set = 0) uniform CameraUBO
{
    mat4 mvp;
    mat4 proj;
    mat4 view;
    mat4 model;
    vec4 fustrums[6];
    vec4 position;
} camera_ubo;

layout (binding = 1, set = 0) uniform SceneUbo
{
    uint modelCount;
    uint iblMipLevels;
} scene_ubo;

#include "include/pbr.h"
#include "include/lights.h"
#include "include/tonemapping.h"
#include "include/shadow.h"

layout (binding = 0, set = 2) buffer CascadeSSbo
{
    CascadeInfo cascades[];
};

uint getCascadeIndex(vec3 position)
{
    uint idx = 0;
    vec3 viewPos = (camera_ubo.view * vec4(position, 1.0)).xyz;
	for (uint i = 0; i < SHADOW_CASCADE_COUNT - 1; ++i)
    {
        if (viewPos.z < cascades[i].splitDepth)
        {	
			idx = i + 1;
		}
	}
    return idx;
}

void main()
{   

    vec3 inPos = texture(PositionSampler, inUv).rgb;
    vec3 baseColour = texture(BaseColourSampler, inUv).rgb;
    vec3 emissive = texture(EmissiveSampler, inUv).rgb;
    
    vec3 colour = baseColour;

    // if lighting isn't applied to this fragment then
    // exit early.
    if (texture(EmissiveSampler, inUv).a == 0.0)
    {
        outFrag = vec4(toneMap(baseColour), 1.0);
        return;
    }

    vec3 V = normalize(camera_ubo.position.xyz - inPos);
    vec3 N = normalize(texture(NormalSampler, inUv).rgb);
    vec3 R = reflect(-V, N);

    float NdotV = clamp(dot(N, V), 0.0, 1.0);

    // get pbr information from G-buffer
    float metallic = texture(PbrSampler, inUv).r;
    float roughness = texture(PbrSampler, inUv).g;
    float occlusion = texture(BaseColourSampler, inUv).a;
    
    // TODO: Add support for specular extension - these values will be updated from the factor/texture.a
    vec3 F0 = vec3(0.04);
    float specularWeight = 1.0;
    vec3 F90 = vec3(1.0);

    float alphaRoughness = roughness * roughness;

    if (HAS_IBL)
    {
        vec3 irradiance = texture(IrradianceEnvMap, N).rgb;
        vec3 diffuse = irradiance * baseColour;

        vec3 metalFresnel = calculateGGXFresnel(NdotV, roughness, baseColour.rgb, 1.0);
        vec3 specularMetal = calculateRadianceGGX(R, roughness, scene_ubo.iblMipLevels, 1.0);
        vec3 metalBrdf = metalFresnel * specularMetal;

        vec3 dielectricFresnel = calculateGGXFresnel(NdotV, roughness, F0, specularWeight);
        vec3 dielectricBrdf = mix(diffuse, specularMetal, dielectricFresnel);

        colour = mix(dielectricBrdf, metalBrdf, metallic);
        // TODO: deal with occlusion strength here?
        colour *= occlusion;
    }

    // TEMP: values for single light until lighting manager update PR.
    LightParams params;
    params.colour = vec4(0.9, 0.2, 0.0, 1.0);
    params.lightType = LIGHT_TYPE_DIRECTIONAL;
    params.pos = vec4(0.0, 2.5, 0.0, 1.0);
    params.direction = vec4(0.0, 0.0, 0.0, 1.0);
    params.offset = 0.0;
    params.fallOut = 0.2;
    params.scale = 1.0;

    // Punctal lighting.
    //for (int idx = 0; idx < LIGHT_COUNT; idx++)
    //{
       // LightParams params = light_ssbo.params[idx];

        vec3 posToLight = params.pos.xyz - inPos;
        vec3 L = normalize(posToLight);

        vec3 intensity = getLightIntensity(params, posToLight, L);    
    
        vec3 H = normalize(L + V);
        float VdotH = clamp(dot(V, H), 0.0, 1.0);
        float NdotL = clamp(dot(N, L), 0.0, 1.0);
        float NdotH = clamp(dot(N, H), 0.0, 1.0);

        vec3 dielectricF = fresnelSchlick(abs(VdotH), F0 * specularWeight, F90);
        vec3 metalF = fresnelSchlick(abs(VdotH), baseColour.rgb, vec3(1.0));

        vec3 diffuse = intensity * NdotL * (baseColour.rgb / PI);

        vec3 specMetal = intensity * NdotL * calculateSpecularGGX(alphaRoughness, NdotL, NdotV, NdotH);
        vec3 metalBrdf = metalF * specMetal;
        vec3 dielectricBrdf = mix(diffuse, specMetal, dielectricF);

        vec3 lightColour = mix(metalBrdf, dielectricBrdf, metallic);
        
        colour += lightColour;

  //  }

    // Apply emission to final colour.
    colour += emissive;

    // Apply shadow mapping.
    uint cascadeIdx = 0;
    if (DRAW_SHADOWS)
    {
        cascadeIdx = getCascadeIndex(inPos);
        vec4 shadowCoord = biasMat * cascades[cascadeIdx].vp * vec4(inPos, 1.0);
        float shadow = filterPCF(shadowCoord / shadowCoord.w, cascadeIdx, shadowMap);
        colour *= shadow;
    }

    // Apply tonemapping (ACES only)
    colour = toneMap(colour);

    outFrag = vec4(colour, 1.0);

    /*switch(cascadeIdx) 
    {
        case 0 : 
            outFrag.rgb *= vec3(1.0f, 0.25f, 0.25f);
            break;
        case 1 : 
            outFrag.rgb *= vec3(0.25f, 1.0f, 0.25f);
            break;
        case 2 : 
            outFrag.rgb *= vec3(0.25f, 0.25f, 1.0f);
            break;
        case 3 : 
            outFrag.rgb *= vec3(1.0f, 1.0f, 0.25f);
            break;
	}*/
}
