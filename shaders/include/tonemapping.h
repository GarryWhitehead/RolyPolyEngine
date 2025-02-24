#ifndef TONEMAPPING_H
#define TONEMAPPING_H

#define GAMMA 2.2
#define INV_GAMMA 1.0 / GAMMA

// From: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 toneMapACES_Narkowicz(vec3 colour)
{
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return clamp((colour * (A * colour + B)) / (colour * (C * colour + D) + E), 0.0, 1.0);
}

// From: http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 linearTosRGB(vec3 colour)
{
    return pow(colour, vec3(INV_GAMMA));
}

vec3 toneMap(vec3 colour)
{
    return linearTosRGB(toneMapACES_Narkowicz(colour));
}

#endif