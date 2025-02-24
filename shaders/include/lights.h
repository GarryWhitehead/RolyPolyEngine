#ifndef LIGHTS_H
#define LIGHTS_H

struct LightParams
{
    mat4 viewMatrix;
    vec4 pos;
    vec4 direction;
    vec4 colour;
    uint lightType;
    float scale;
    float offset;
    float fallOut;
};

// Taken from: https://github.com/google/filament/blob/main/shaders/src/surface_light_punctual.fs
float calculateAngle(vec3 lightDir, vec3 L, float scale, float offset)
{
    float angle = dot(lightDir, L);
    float attenuation = clamp(angle * scale + offset, 0.0, 1.0);
    return attenuation * attenuation;
}

// Taken from: https://github.com/google/filament/blob/main/shaders/src/surface_light_punctual.fs 
float calculateDistance(vec3 posToLight, float fallOut)
{
    float dist = dot(posToLight, posToLight);
    float factor = dist * fallOut;
    float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0);
    float smoothFactor2 = smoothFactor * smoothFactor;
    return smoothFactor2 / max(dist, 1e-4);
}

vec3 calculateSunArea(vec3 direction, vec3 sunPosition, vec3 R)
{
    float LdotR = dot(direction, R);
    vec3 s = R - LdotR * direction;
    float d = sunPosition.x;
    return LdotR < d ? normalize(direction * d + normalize(s) * sunPosition.y)
                     : R;
}

vec3 getLightIntensity(LightParams params, vec3 posToLight, vec3 L)
{
    float attenuation = 1.0;
    float intensity = params.colour.a;

    if (params.lightType != LIGHT_TYPE_DIRECTIONAL)
    {

        attenuation = calculateDistance(posToLight, params.fallOut);

        if (params.lightType == LIGHT_TYPE_SPOT)
        {
            attenuation *= calculateAngle(
                -params.direction.xyz, L, params.scale, params.offset);
        }
    }

    return attenuation * intensity * params.colour.rgb;
}

#endif
