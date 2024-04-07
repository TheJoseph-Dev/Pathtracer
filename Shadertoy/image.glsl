void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    
    vec4 color = texture(iChannel0, fragCoord/iResolution.xy);
    #ifdef ACCUMULATE
    color /= float(iFrame+1);
    #endif
    fragColor = color;
}