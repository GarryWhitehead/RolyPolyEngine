#ifndef SHADOW_H
#define SHADOW_H

struct CascadeInfo
{
    mat4 vp;
    float splitDepth;
};

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex, sampler2DArray map)
{
	float shadow = 1.0;
	float bias = 0.005;

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
    {
		float dist = texture(map, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) 
        {
			// ambient value.
            shadow = 0.3;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc, uint cascadeIndex, sampler2DArray map)
{
	ivec2 texDim = textureSize(map, 0).xy;
	float scale = 0.75;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++) 
    {
		for (int y = -range; y <= range; y++) 
        {
			shadowFactor += textureProj(sc, vec2(dx * x, dy * y), cascadeIndex, map);
			count++;
		}
	}
	return shadowFactor / count;
}

#endif