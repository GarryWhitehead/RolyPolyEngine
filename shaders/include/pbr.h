#ifndef PBR_H
#define PBR_H

#include "math.h"

// Microfacet distribution across the surface.
float GGX_Distribution(float NdotH, float roughness)
{
    float r2 = roughness * roughness;
    float denom = (NdotH * NdotH) * (r2 - 1.0) + 1.0;
    return r2 / (PI * denom * denom);
}

// Specular geometric attenuation. From: "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs", Heitz (2014)
float GeometryShlickGGX(float NdotV, float NdotL, float roughness)
{
    float r2 = roughness * roughness;
    float GL = NdotL * sqrt((NdotV - r2 * NdotV) * NdotV + r2);
    float GV = NdotV * sqrt((NdotL - r2 * NdotL) * NdotL + r2);
    float GGX = GV + GL + 0.0001;
    return 0.5 / GGX;
}

// Fresnal reflectance part part of the specular equation
vec3 fresnelSchlick(float VdotH, vec3 specReflectance, vec3 specReflectance90)
{
    return specReflectance +
        (specReflectance90 - specReflectance) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

vec3 FresnelRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// From: A Multiple-Scattering Microfacet Model for Real-Time Image-based Lighting; Carmelo J. Fdez-Aguera
vec3 calculateGGXFresnel(float NdotV, float roughness, vec3 F0, float specularWeight)
{
    vec2 brdfUv = clamp(vec2(NdotV, roughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
    vec2 f_ab = texture(BrdfSampler, brdfUv).rg;
    vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
    vec3 k_S = F0 + Fr * pow(1.0 - NdotV, 5.0);
    vec3 FssEss = specularWeight * (k_S * f_ab.x + f_ab.y);

    // Fresnel modification from the above paper.
    float Ems = (1.0 - (f_ab.x + f_ab.y));
    vec3 F_avg = specularWeight * (F0 + (1.0 - F0) / 21.0);
    vec3 FmsEms = Ems * FssEss * F_avg / (1.0 - F_avg * Ems);

    return FssEss + FmsEms;
}

vec3 calculateRadianceGGX(vec3 R, float roughness, uint mipCount, float envIntensity)
{
    float lod = roughness * float(mipCount - 1);
    vec4 textureSample = textureLod(SpecularEnvMap, R, lod);
    return textureSample.rgb *= envIntensity;
}

vec3 calculateSpecularGGX(float alphaRoughness, float NdotL, float NdotV, float NdotH)
{
    float vis = GeometryShlickGGX(NdotV, NdotL, alphaRoughness);
    float D = GGX_Distribution(NdotH, alphaRoughness);

    return vec3(vis * D);
}

#endif // PBR_H