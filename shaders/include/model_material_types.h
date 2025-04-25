#ifndef MODEL_MATERIAL_TYPES_H
#define MODEL_MATERIAL_TYPES_H

#include "common.h"

void defaultMaterial() { outPos = vec4(inPos, 1.0); }

void skyboxMaterial(DrawData drawData)
{
    // Skybox index stored in the base colour.
    vec3 worldUv = inPos.xyz;
    worldUv.y *= -1.0;
    vec4 skyColour = texture(cubeMaps[nonuniformEXT(drawData.colour)], worldUv);
    outColour = skyColour;

    // Zero the emissive texture alpha channel as this tells the
    // lighting shader to skip applying lighting to the skybox.
    outEmissive.a = 0.0;
}

void uiMaterial() 
{ 
    outPos = vec4(inPos.xy, 0.0, 1.0); 
    outColour = inColour * texture(cubeMaps[nonuniformEXT(drawData.colour)], worldUv);
}

void modelFragment(DrawData drawData)
{
    switch (MATERIAL_TYPE)
    {
        case MATERIAL_TYPE_DEFAULT: {
            defaultMaterial();
            break;
        }
        case MATERIAL_TYPE_SKYBOX: {
            skyboxMaterial(drawData);
            break;
        }
        case MATERIAL_TYPE_UI: {
            uiMaterial();
            break;
        }
    }
}

#endif