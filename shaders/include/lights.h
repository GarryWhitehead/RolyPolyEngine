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

float calculateAngle(vec3 lightDir, vec3 L, float scale, float offset)
{
    float angle = dot(lightDir, L);
    float attenuation = clamp(angle * scale + offset, 0.0, 1.0);
    return attenuation * attenuation;
}

float calculateDistance(vec3 L, float fallOut)
{
    float dist = dot(L, L);
    float factor = dist * fallOut;
    float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0);
    float smoothFactor2 = smoothFactor * smoothFactor;
    return smoothFactor2 * 1.0 / max(dist, 1e-4);
}

vec3 calculateSunArea(vec3 direction, vec3 sunPosition, vec3 R)
{
    float LdotR = dot(direction, R);
    vec3 s = R - LdotR * direction;
    float d = sunPosition.x;
    return LdotR < d ? normalize(direction * d + normalize(s) * sunPosition.y)
                     : R;
}

#endif
