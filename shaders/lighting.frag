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

layout (constant_id = 0) const bool HAS_IBL = false;
layout (constant_id = 1) const uint LIGHT_COUNT = 0;


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

//layout (constant_id = 0) const int LIGHT_COUNT = 0;

void main()
{   

    vec3 inPos = texture(PositionSampler, inUv).rgb;
    vec3 baseColour = texture(BaseColourSampler, inUv).rgb;
    vec3 emissive = texture(EmissiveSampler, inUv).rgb;

    vec3 colour = baseColour;

    // if lighting isn't applied to this fragment then
    // exit early.
    /*if (applyLightingFlag == 0.0)
    {
        outFrag = vec4(baseColour, 1.0);
        return;
    }*/

    vec3 V = normalize(camera_ubo.position.xyz - inPos);
    vec3 N = normalize(texture(NormalSampler, inUv).rgb);
    vec3 R = normalize(reflect(-V, N));

    float NdotV = clamp(dot(N, V), 0.0, 1.0);

    // get pbr information from G-buffer
    float metallic = texture(PbrSampler, inUv).r;
    float roughness = texture(PbrSampler, inUv).g;
    float occlusion = texture(BaseColourSampler, inUv).a;
    
    
    // TODO: Add support for specular extension - these values will be updated from the factor/texture.a
    vec3 F0 = vec3(0.04);
    float specularWeight = 1.0;
    vec3 F90 = vec3(1.0);

    /*vec3 specularColour = mix(F0, baseColour, metallic);

    float reflectance =
        max(max(specularColour.r, specularColour.g), specularColour.b);
    float reflectance90 =
        clamp(reflectance * 25.0, 0.0, 1.0); // 25.0-50.0 is used
    vec3 specReflectance = specularColour.rgb;
    vec3 specReflectance90 = vec3(1.0, 1.0, 1.0) * reflectance90;*/

    float alphaRoughness = roughness * roughness;

    if (HAS_IBL)
    {
        vec3 irradiance = texture(IrradianceEnvMap, N).rgb;
        vec3 diffuse = irradiance * baseColour;

        vec3 metalFresnel = calculateGGXFresnel(NdotV, roughness, baseColour.rgb, 1.0);
        vec3 specularMetal = calculateRadianceGGX(NdotV, R, roughness, scene_ubo.iblMipLevels, 1.0);
        vec3 metalBrdf = metalFresnel * specularMetal;

        vec3 dielectricFresnel = calculateGGXFresnel(NdotV, roughness, F0, specularWeight);
        vec3 dielectricBrdf = mix(diffuse, specularMetal, dielectricFresnel);

        colour = mix(dielectricBrdf, metalBrdf, metallic);
        // TODO: deal with occlusion strength here?
        colour *= occlusion;
    }

    // TEMP values for single light.
    LightParams params;
    params.colour = vec4(0.8, 0.2, 0.0, 0.5);
    params.lightType = LIGHT_TYPE_DIRECTIONAL;
    params.pos = vec4(0.0, 2.5, 0.0, 1.0);
    params.direction = vec4(0.0, 0.0, 0.0, 1.0);
    params.offset = 0.0;
    params.fallOut = 0.5;
    params.scale = 1.0;

    // apply additional lighting contribution to specular
    
    float attenuation = 0.0;
    float intensity = 0.0;
    vec3 L = vec3(0.0);
    vec3 lightPos = vec3(0.0);

    //for (int idx = 0; idx < LIGHT_COUNT; idx++)
    //{
       // LightParams params = light_ssbo.params[idx];

       /* if (params.lightType != LIGHT_TYPE_DIRECTIONAL)
        {
            lightPos = params.pos.xyz - inPos;
            L = normalize(lightPos);
            intensity = params.colour.a;
            attenuation = calculateDistance(lightPos, params.fallOut);

            if (params.lightType == LIGHT_TYPE_SPOT)
            {
                attenuation *= calculateAngle(
                    params.direction.xyz, L, params.scale, params.offset);
            }
        }
        else
        {
            L = calculateSunArea(params.direction.xyz, params.pos.xyz, R);
            intensity = params.colour.a;
            attenuation = 1.0f;
        }

        colour += specularContribution(
            L,
            V,
            N,
            baseColour,
            metallic,
            alphaRoughness,
            attenuation,
            intensity,
            params.colour.rgb,
            specReflectance,
            specReflectance90);*/
  //  }


   
     
  /*  }
    else
    {
        // TODO: Temp measure until IBL/BRDF added.
        float ambient = 0.4;
        vec3 diffuse = max(dot(N, L), ambient).rrr;
        float specular = pow(max(dot(R, V), 0.0), 32.0);
        finalColour = diffuse * colour.rgb + specular;
        
        // Apply occlusion to final colour.
        finalColour = mix(finalColour, finalColour * occlusion, 1.0);
    }*/

    // Apply emission to final colour.
    colour += emissive;

    // TODO: The final colour needs to take into consideration the base colour alpha channel.
    outFrag = vec4(colour, 1.0);
}
