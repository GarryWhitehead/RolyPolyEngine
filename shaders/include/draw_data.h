#ifndef DRAW_DATA_H
#define DRAW_DATA_H

#include "common.h"

#define METALLIC_ROUGHNESS_PIPELINE     0
#define SPECULAR_GLOSSINESS_PIPELINE    1
#define NO_PIPELINE_WORKFLOW            2

struct DrawData
{
    vec4 emissiveFactor;
    vec4 baseColourFactor;
    vec4 diffuseFactor;
    vec4 specularFactor;
    float alphaMaskCutOff;
    float alphaMask;
    float roughnessFactor;
    float metallicFactor;
    // Texture indices into the bindless samppler.
    // Note: the order here is dictated by the TextureType enum.
    uint colour;
    uint normal;
    uint mr;
    uint diffuse;
    uint emissive;
    uint occlusion;
    // uv indices
    uint colourUv;
    uint normalUv;
    uint mrUv;
    uint diffuseUv;
    uint emissiveUv;
    uint occlusionUv;
};

#endif