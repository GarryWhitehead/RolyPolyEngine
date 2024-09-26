[[vertex]]

[[end]]

[[fragment]]
void postprocessFragment()
{
    // TODO: add the actual bloom process!
    vec3 colour = texture(ColourSampler, inUv).rgb; 

    // use the averaged luminance value for the exposure
    float exposure = texture(LuminanceAvgLut, vec2(0.0,0.0)).r;
    vec3 result = vec3(1.0) - exp(-colour * exposure);
     
    result = pow(result, vec3(1.0 / material_ubo.gamma));

    outColour = vec4(result, 1.0);
}
[[end]]
