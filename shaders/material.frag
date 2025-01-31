#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "include/math.h"


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

layout(location = 0) in vec2 inUv0;
layout(location = 1) in vec2 inUv1;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec4 inColour;
layout(location = 5) flat in uint inModelDrawIdx;
layout(location = 6) in vec3 inPos;

layout(location = 0) out vec4 outColour;
layout(location = 1) out vec4 outPos;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outEmissive;
layout(location = 4) out vec2 outPbr;

layout (constant_id = 0) const bool HAS_ALPHA_MASK = false;
layout (constant_id = 1) const bool HAS_BASECOLOUR_SAMPLER = false;
layout (constant_id = 2) const bool HAS_APLHA_MASK_CUTOFF = false;
layout (constant_id = 3) const int PIPELINE_WORKFLOW = NO_PIPELINE_WORKFLOW;
layout (constant_id = 4) const bool HAS_MR_SAMPLER = false;
layout (constant_id = 5) const bool HAS_DIFFUSE_SAMPLER = false;
layout (constant_id = 6) const bool HAS_DIFFUSE_FACTOR = false;
layout (constant_id = 7) const bool HAS_NORMAL_SAMPLER = false;
layout (constant_id = 8) const bool HAS_OCCLUSION_SAMPLER = false;
layout (constant_id = 9) const bool HAS_EMISSIVE_SAMPLER = false;
layout (constant_id = 10) const bool HAS_UV = false;
layout (constant_id = 11) const bool HAS_NORMAL = false;
layout (constant_id = 12) const bool HAS_TANGENT = false;
layout (constant_id = 13) const bool HAS_COLOUR_ATTR = false;


layout(set = 3, binding = 0) uniform sampler2D textures[];
//layout(set = 3, binding = 0) uniform samplerCube texCubes[];

layout (set = 2, binding = 2) buffer MeshDataSsbo
{
    DrawData drawData[];
};

float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular)
{
    float perceivedDiffuse = sqrt(
        0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g +
        0.114 * diffuse.b * diffuse.b);
    float perceivedSpecular = sqrt(
        0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g +
        0.114 * specular.b * specular.b);

    if (perceivedSpecular < 0.04)
    {
        return 0.0;
    }

    float a = 0.04;
    float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - 0.04) +
        perceivedSpecular - 2.0 * 0.04;
    float c = 0.04 - perceivedSpecular;
    float D = max(b * b - 4.0 * a * c, 0.0);

    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

// Taken from here:
// http://www.thetenthplanet.de/archives/1180
vec3 peturbNormal(vec2 tex_coord, vec3 tangentNormal)
{
    vec3 q1 = dFdx(inPos); // edge1
    vec3 q2 = dFdy(inPos); // edge2
    vec2 st1 = dFdx(tex_coord); // uv1
    vec2 st2 = dFdy(tex_coord); // uv2

    vec3 N = normalize(inNormal);
    vec3 T = normalize(q1 * st2.t - q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{
    DrawData iData = drawData[inModelDrawIdx];

    outPos = vec4(inPos, 1.0);

    // Normal.
    if (HAS_NORMAL_SAMPLER && HAS_UV)
    {
        vec2 uv = iData.normalUv == 0 ? inUv0 : inUv1;
        vec3 norm = texture(textures[nonuniformEXT(iData.normal)], uv).xyz * 2.0 - 1.0;

        if (HAS_NORMAL && HAS_TANGENT)
        {
            vec3 N = normalize(inNormal);
	        vec3 T = normalize(inTangent.xyz);
	        vec3 B = cross(inNormal, inTangent.xyz) * inTangent.w;
	        mat3 TBN = mat3(T, B, N);
            outNormal = vec4(TBN * norm, 1.0);
        }
        else
        {
            outNormal = vec4(peturbNormal(uv, norm), 1.0);
        }
    }
    else if (HAS_NORMAL)
    {
        outNormal = vec4(normalize(inNormal), 1.0);
    }
    else
    {
        outNormal = vec4(normalize(cross(dFdx(inPos), dFdy(inPos))), 1.0);
    }


    // albedo
    vec4 baseColour = vec4(1.0);

    
	if (HAS_ALPHA_MASK)
    {
	  // TODO: If has alpha mask value, only "OPAQUE" and "MASK" are dealt with. Need to do something when "BLEND"?
      // alphaMsak == 1.0 == "MASK"; alphaMask == 0.0 == "OPAQUE"
      // We don't have to do anything for "OPAQUE".
      if (iData.alphaMask == 1.0)
      { 
        float cutoff = HAS_APLHA_MASK_CUTOFF ? iData.alphaMaskCutOff : 0.5;
        if (baseColour.a < cutoff)
        {
            discard;
        }
      }
    }
    
    // default values for output attachments
    float roughness = 1.0;
    float metallic = 1.0;

    switch (PIPELINE_WORKFLOW)
    {
        case METALLIC_ROUGHNESS_PIPELINE:
        {
            roughness = iData.roughnessFactor;
            metallic = iData.metallicFactor;

            if (HAS_MR_SAMPLER && HAS_UV)
            {
                vec2 uv = iData.mrUv == 0 ? inUv0 : inUv1;
                vec3 mrSample = texture(textures[nonuniformEXT(iData.mr)], uv).rgb;
                roughness = clamp(mrSample.g * roughness, 0.0, 1.0);
                metallic = clamp(mrSample.b * metallic, 0.0, 1.0);
            }
            else
            {
                roughness = clamp(roughness, 0.04, 1.0);
                metallic = clamp(metallic, 0.0, 1.0);
            }

            // If the model doesn't define a base colour sampler, then  the base colour factor must be set.
            if (HAS_BASECOLOUR_SAMPLER && HAS_UV)
            {
                vec2 uv = iData.colourUv == 0 ? inUv0 : inUv1;
                baseColour = texture(textures[nonuniformEXT(iData.colour)], uv);
            }
            else
            {
                baseColour = iData.baseColourFactor;
            }
            break;
        }
        
        case SPECULAR_GLOSSINESS_PIPELINE:
        {
            // Values from specular glossiness workflow are converted to metallic
            // roughness
            vec4 diffuse = vec4(0.0);
            vec3 specular = vec3(0.0);;

            if (HAS_MR_SAMPLER  && HAS_UV)
            {
                vec2 uv = iData.mrUv == 0 ? inUv0 : inUv1;
                roughness = 1.0 - texture(textures[nonuniformEXT(iData.mr)], uv).a;
                specular = texture(textures[nonuniformEXT(iData.mr)], uv).rgb;
            }
            
            if (HAS_DIFFUSE_SAMPLER && HAS_UV)
            {
                vec2 uv = iData.diffuseUv == 0 ? inUv0 : inUv1;
                diffuse = texture(textures[nonuniformEXT(iData.diffuse)], uv);
            }
            else if (HAS_DIFFUSE_FACTOR)
            {
                diffuse = iData.diffuseFactor;
            }

            float maxSpecular = max(max(specular.r, specular.g), specular.b);

            // Convert metallic value from specular glossiness inputs
            metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);

            const float minRoughness = 0.04;
            vec3 baseColourDiffusePart = diffuse.rgb *
                ((1.0 - maxSpecular) / (1 - minRoughness) /
                max(1 - metallic, EPSILON)) *
                iData.diffuseFactor.rgb;
            vec3 baseColourSpecularPart = specular -
                (vec3(minRoughness) * (1 - metallic) * (1 / max(metallic, EPSILON))) *
                    iData.specularFactor.rgb;
            baseColour = vec4(
                mix(baseColourDiffusePart, baseColourSpecularPart, metallic * metallic),
                diffuse.a);
             break;
        }
    }

    // =========== fragment outputs ====================

    if (HAS_COLOUR_ATTR)
    {   
        baseColour = inColour;
    }

    outColour = baseColour;
    outPbr = vec2(metallic, roughness);

    // occlusion
    if (HAS_OCCLUSION_SAMPLER && HAS_UV)
    {
        // Bake the occlusion value into the alpha channel of the colour - applied in the lighting stage.
        vec2 uv = iData.occlusionUv == 0 ? inUv0 : inUv1;
        outColour.a = texture(textures[nonuniformEXT(iData.occlusion)], uv).r;
    }

    // emmisive
    vec3 emissive = iData.emissiveFactor.rgb;
    if (HAS_EMISSIVE_SAMPLER && HAS_UV)
    {
        vec2 uv = iData.emissiveUv == 0 ? inUv0 : inUv1;
        emissive *= texture(textures[nonuniformEXT(iData.emissive)], uv).rgb;
    }
    outEmissive = vec4(emissive, 1.0);
}
