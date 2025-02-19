#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_debug_printf : enable

#include "include/math.h"

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec4 outColour;

layout(set = 3, binding = 0) uniform samplerCube BaseColourSampler;

void main()
{
    vec3 N = normalize(inPos);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = normalize(cross(N, right));
	
	vec3 irrColour = vec3(0.0);
	uint sampleCount = 0;
		
	const float doublePI = PI * 2;

	const float dPhi = 0.035;
	const float dTheta = 0.025;
	
	for(float phi = 0.0; phi < doublePI; phi += dPhi) 
    {
		for(float theta = 0.0; theta < HALF_PI; theta += dTheta) 
        {			
			// spherical to cartesian
			vec3 tanSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			
			// to world space
			vec3 wPos = tanSample.x * right + tanSample.y * up + tanSample.z * N;
		
			irrColour += texture(BaseColourSampler, wPos).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}
	outColour = vec4(PI * irrColour / float(sampleCount), 1.0);
}