#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "include/math.h"

#define SPECULAR_GLOSSINESS_PIPELINE 0
#define METALLIC_ROUGHNESS_PIPELINE 1

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
    uint colour;
    uint normal;
    uint mr;
    uint diffuse;
    uint emissive;
    uint occlusion;
} draw_data;

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColour;
layout(location = 3) flat in uint inModelDrawIdx;
layout(location = 4) in vec3 inPos;

layout(location = 0) out vec4 outColour;
layout(location = 1) out vec4 outPos;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outEmissive;
layout(location = 4) out vec2 outPbr;

layout (constant_id = 0) const int HAS_ALPHA_MASK = 0;
layout (constant_id = 1) const int HAS_BASECOLOUR_SAMPLER = 0;
layout (constant_id = 2) const int HAS_BASECOLOUR_FACTOR = 0;
layout (constant_id = 3) const int HAS_APLHA_MASK_CUTOFF = 0;
layout (constant_id = 4) const int PIPELINE_WORKFLOW = 0;
layout (constant_id = 5) const int HAS_MR_SAMPLER = 0;
layout (constant_id = 6) const int HAS_DIFFUSE_SAMPLER = 0;
layout (constant_id = 7) const int HAS_DIFFUSE_FACTOR = 0;
layout (constant_id = 8) const int HAS_NORMAL_SAMPLER = 0;
layout (constant_id = 9) const int HAS_OCCLUSION_SAMPLER = 0;
layout (constant_id = 10) const int HAS_EMISSIVE_SAMPLER = 0;
layout (constant_id = 11) const int HAS_EMISSIVE_FACTOR = 0;
layout (constant_id = 12) const int HAS_UV = 0;
layout (constant_id = 13) const int HAS_NORMAL = 0;
layout (constant_id = 14) const int HAS_COLOUR_ATTR = 0;


layout(set = 3, binding = 0) uniform sampler2D textures[];
//layout(set = 3, binding = 0) uniform samplerCube texCubes[];


layout (set = 2, binding = 2) buffer MeshDataSsbo
{
    DrawData drawData[];
};

#define EPSILON 0.0000001

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
    if (HAS_NORMAL_SAMPLER == 1 && HAS_UV == 1)
    {
        // convert normal to -1, 1 coord system
        vec3 tangentNormal = texture(textures[nonuniformEXT(iData.normal)], inUv).xyz * 2.0 - 1.0;
        outNormal = vec4(peturbNormal(inUv, tangentNormal), 1.0);
    }
    else if (HAS_NORMAL == 1)
    {
        outNormal = vec4(normalize(inNormal), 1.0);
    }
    else
    {
        outNormal = vec4(normalize(cross(dFdx(inPos), dFdy(inPos))), 1.0);
    }
    if (HAS_COLOUR_ATTR == 1)
    {   
        outColour = inColour;
        // Early return as we don't care about PBR if using vertex colour.
        return;
    }
    
    // albedo
    vec4 baseColour = vec4(1.0);
    float alphaMask = 1.0;

	if (HAS_ALPHA_MASK == 1)
    {
	  if (HAS_APLHA_MASK_CUTOFF == 1 && baseColour.a < draw_data.alphaMaskCutOff)
	  {
        discard;
	  }
	  alphaMask = draw_data.alphaMask;
    }

    if (alphaMask == 1.0)
    {
        if (HAS_BASECOLOUR_SAMPLER == 1 && HAS_UV == 1)
        {
            baseColour = texture(textures[nonuniformEXT(iData.colour)], inUv);

            if (HAS_BASECOLOUR_FACTOR == 1)
            {
                baseColour *= draw_data.baseColourFactor;
            }
        }
        else if (HAS_BASECOLOUR_FACTOR == 1)
        {
            baseColour = draw_data.baseColourFactor;
        }
    }
    
    // default values for output attachments
    float roughness = 1.0;
    float metallic = 1.0;


    switch(PIPELINE_WORKFLOW)
    {
        case SPECULAR_GLOSSINESS_PIPELINE:
        {
            roughness = draw_data.roughnessFactor;
            metallic = draw_data.metallicFactor;

            if (HAS_MR_SAMPLER == 1 && HAS_UV == 1)
            {
                vec4 mrSample = texture(textures[nonuniformEXT(iData.mr)], inUv);
                roughness = clamp(mrSample.g * roughness, 0.0, 1.0);
                metallic = mrSample.b * metallic;
            }
            else
            {
                roughness = clamp(roughness, 0.04, 1.0);
                metallic = clamp(metallic, 0.0, 1.0);
            }
            break;
        }
        case METALLIC_ROUGHNESS_PIPELINE:
        {
            // Values from specular glossiness workflow are converted to metallic
            // roughness
            vec4 diffuse;
            vec3 specular;

            if (HAS_MR_SAMPLER == 1  && HAS_UV == 1)
            {
                roughness = 1.0 - texture(textures[nonuniformEXT(iData.mr)], inUv).a;
                specular = texture(textures[nonuniformEXT(iData.mr)], inUv).rgb;
            }
            else
            {
                roughness = 1.0;
                specular = vec3(0.0);
            }
        

            if (HAS_DIFFUSE_SAMPLER == 1  && HAS_UV == 1)
            {
                diffuse = texture(textures[nonuniformEXT(iData.diffuse)], inUv);
            }
            else if (HAS_DIFFUSE_FACTOR == 1)
            {
                diffuse = draw_data.diffuseFactor;
            }

            float maxSpecular = max(max(specular.r, specular.g), specular.b);

            // Convert metallic value from specular glossiness inputs
            metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);

            const float minRoughness = 0.04;
            vec3 baseColourDiffusePart = diffuse.rgb *
                ((1.0 - maxSpecular) / (1 - minRoughness) /
                max(1 - metallic, EPSILON)) *
                draw_data.diffuseFactor.rgb;
            vec3 baseColourSpecularPart = specular -
                (vec3(minRoughness) * (1 - metallic) * (1 / max(metallic, EPSILON))) *
                    draw_data.specularFactor.rgb;
            baseColour = vec4(
                mix(baseColourDiffusePart, baseColourSpecularPart, metallic * metallic),
                diffuse.a);
            break;
        }
    }

    // =========== fragment outputs ====================

    outColour = baseColour;
    outPbr = vec2(metallic, roughness);

    // ao
    float ambient = 0.4;
    if (HAS_OCCLUSION_SAMPLER == 1 && HAS_UV == 1)
    {
        ambient = texture(textures[nonuniformEXT(iData.occlusion)], inUv).x;
    }
    outColour.a = ambient;

    // emmisive
    vec3 emissive = vec3(0.2);
    if (HAS_EMISSIVE_SAMPLER == 1 && HAS_UV == 1)
    {
        emissive = texture(textures[nonuniformEXT(iData.emissive)], inUv).rgb;
        emissive *= draw_data.emissiveFactor.rgb;
    }
    else if (HAS_EMISSIVE_FACTOR == 1)
    {
        emissive = draw_data.emissiveFactor.rgb;
    }
    outEmissive = vec4(emissive, 1.0);
}
