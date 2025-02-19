#ifndef PBR_H
#define PBR_H

#include "math.h"

// microfacet distribution across the surface
float GGX_Distribution(float NdotH, float roughness)
{
    float r2 = roughness * roughness;
    float denom = (NdotH * r2 - NdotH) * NdotH + 1.0;
    return r2 / (PI * denom * denom);
}

// specular geometric attenuation - rougher surfaces reflect less light
float GeometryShlickGGX(float NdotV, float NdotL, float roughness)
{
    float r2 = roughness * roughness;
    float GL = 2.0 * NdotL / (NdotL + sqrt(r2 + (1.0 - r2) * (NdotL * NdotL)));
    float GV = 2.0 * NdotV / (NdotV + sqrt(r2 + (1.0 - r2) * (NdotV * NdotV)));
    return GL * GV;
}

// Fresnal reflectance part part of the specular equation
vec3 FresnelSchlick(float VdotH, vec3 specReflectance, vec3 specReflectance90)
{
    return specReflectance +
        vec3(specReflectance90 - specReflectance) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
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

vec3 calculateRadianceGGX(float NdotV, vec3 R, float roughness, uint mipCount, float envIntensity)
{
    float lod = roughness * float(mipCount - 1);
    vec4 textureSample = textureLod(SpecularEnvMap, R, lod);
    return textureSample.rgb *= envIntensity;
}

vec3 specularContribution(
    vec3 L,
    vec3 V,
    vec3 N,
    vec3 albedo,
    float metallic,
    float alphaRoughness,
    float attenuation,
    float intensity,
    vec3 radiance,
    vec3 specReflectance,
    vec3 specReflectance90)
{
    vec3 H = normalize(V + L);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float NdotV = clamp(dot(N, V), 0.001, 1.0);
    float NdotL = clamp(dot(N, L), 0.001, 1.0);
    float VdotH = clamp(dot(V, H), 0.0, 1.0);

    vec3 colour = vec3(0.0);

    float D = GGX_Distribution(NdotH, alphaRoughness);
    float G = GeometryShlickGGX(NdotV, NdotL, alphaRoughness);
    vec3 F = FresnelSchlick(VdotH, specReflectance, specReflectance90);

    vec3 specularContribution = F * G * D / (4.0 * NdotL * NdotV);
    vec3 diffuseContribution = (vec3(1.0) - F) * (albedo / PI);
    colour = diffuseContribution + specularContribution;

    return (colour * radiance.xyz) * (intensity * attenuation * NdotL);
}

#endif // PBR_H