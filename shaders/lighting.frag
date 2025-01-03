#version 460

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outFrag;

layout(set = 3, binding = 0) uniform sampler2D PositionSampler;
layout(set = 3, binding = 1) uniform sampler2D BaseColourSampler;
layout(set = 3, binding = 2) uniform sampler2D NormalSampler;
layout(set = 3, binding = 3) uniform sampler2D PbrSampler;
layout(set = 3, binding = 4) uniform sampler2D EmissiveSampler;

#define LightTypePointId 0
#define LightTypeSpotId 1
#define LightTypeDir 2

//layout (constant_id = 0) const int LIGHT_COUNT = 0;

/*#if defined(IBL_ENABLED)
vec3 calculateIBL(vec3 N, float NdotV, float roughness, vec3 reflection, vec3 diffuseColour, vec3 specularColour)
{	
	vec3 bdrf = (texture(BrdfSampler, vec2(NdotV, 1.0 - roughness))).rgb;
	
	// specular contribution
	const float maxLod = scene_ubo.iblMipLevels;
	
	float lod = maxLod * roughness;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	
	vec3 a = textureLod(SpecularSampler, reflection, lodf).rgb;
	vec3 b = textureLod(SpecularSampler, reflection, lodc).rgb;
	vec3 specularLight = mix(a, b, lod - lodf);
	
	vec3 specular = specularLight * (specularColour * bdrf.x + bdrf.y);
	
	// diffuse contribution
	vec3 diffuseLight = texture(IrradianceSampler, N).rgb;
	vec3 diffuse = diffuseLight * diffuseColour;
	
	return diffuse + specular;
}
#endif*/

void main()
{
    vec3 inPos = texture(PositionSampler, inUv).rgb;
    vec3 baseColour = texture(BaseColourSampler, inUv).rgb;
    float applyLightingFlag = texture(EmissiveSampler, inUv).a;

    // if lighting isn't applied to this fragment then
    // exit early.
    /*if (applyLightingFlag == 0.0)
    {
        outFrag = vec4(baseColour, 1.0);
        return;
    }

    vec3 V = normalize(scene_ubo.position.xyz - inPos);
    vec3 N = texture(NormalSampler, inUv).rgb;
    vec3 R = reflect(-V, N);

    // get pbr information from G-buffer
    float metallic = texture(PbrSampler, inUv).x;
    float roughness = texture(PbrSampler, inUv).y;
    float occlusion = texture(BaseColourSampler, inUv).a;
    vec3 emissive = texture(EmissiveSampler, inUv).rgb;

    vec3 F0 = vec3(0.04);
    vec3 specularColour = mix(F0, baseColour, metallic);

    float reflectance =
        max(max(specularColour.r, specularColour.g), specularColour.b);
    float reflectance90 =
        clamp(reflectance * 25.0, 0.0, 1.0); // 25.0-50.0 is used
    vec3 specReflectance = specularColour.rgb;
    vec3 specReflectance90 = vec3(1.0, 1.0, 1.0) * reflectance90;

    float alphaRoughness = roughness * roughness;*/

    // apply additional lighting contribution to specular
    //vec3 colour = baseColour;

    /*for (int idx = 0; idx < LIGHT_COUNT; idx++)
    {
        LightParams params = light_ssbo.params[idx];
        if (params.lightType == BufferEndSignal)
        {
            break;
        }

        if (params.lightType == LightTypePoint ||
            params.lightType == LightTypeSpot)
        {
            vec3 lightPos = params.pos.xyz - inPos;
            vec3 L = normalize(lightPos);
            float intensity = params.colour.a;

            float attenuation = calculateDistance(lightPos, params.fallOut);
            if (params.lightType == LightTypeSpot)
            {
                attenuation *= calculateAngle(
                    params.direction.xyz, L, params.scale, params.offset);
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
                specReflectance90);
        }
        else if (params.lightType == LightTypeDir)
        {
            vec3 L = calculateSunArea(params.direction.xyz, params.pos.xyz, R);
            float intensity = params.colour.a;
            float attenuation = 1.0f;
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
                specReflectance90);
        }
    }*/

    // add the ibl contribution to the fragment
	/*float NdotV = max(dot(N, V), 0.0);
	
    vec3 F = FresnelRoughness(NdotV, F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance = texture(IrradianceSampler, N).rgb;
    vec3 diffuse = irradiance * baseColour;
    
    const float maxLod = scene_ubo.iblMipLevels;
    vec3 prefilteredColor = textureLod(SpecularSampler, R,  roughness * maxLod).rgb;    
    vec2 brdf = texture(BrdfSampler, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * occlusion;
    vec3 finalColour = colour + ambient;
#else
    // occlusion
    vec3 finalColour = mix(colour, colour * occlusion, 1.0);
#endif

    // emissive
    finalColour += emissive;*/

    outFrag = vec4(baseColour, 1.0);
    //outFrag = vec4(finalColour, 1.0);
}
