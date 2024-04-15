#VERTEX_SHADER
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoords;

uniform mat4 MVP;

out vec2 texCoord;

void main() {
	texCoord = texCoords;
	texCoord.y = 1.0-texCoords.y;
	gl_Position = MVP*vec4(position.xy, 0.0, 1.0);
}

#FRAGMENT_SHADER
#version 460 core
// This shader performs upsampling on a texture,
// as taken from Call Of Duty method, presented at ACM Siggraph 2014.

// Remember to add bilinear minification filter for this texture!
// Remember to use a floating-point texture format (for HDR)!
// Remember to use edge clamping for this texture!
uniform sampler2D srcTexture;
uniform float filterRadius;

in vec2 texCoord;
layout (location = 0) out vec3 upsample;

void main()
{
     //vec4 fullTexColor = texture(srcTexture,texCoord);
     //if (fullTexColor.r < 5.1 && fullTexColor.g < 5.1 && fullTexColor.b < 5.1) { upsample = vec3(0.0); return; }

    // The filter kernel is applied with a radius, specified in texture
    // coordinates, so that the radius will vary across mip resolutions.
    float x = filterRadius;
    float y = filterRadius;

    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(srcTexture, vec2(texCoord.x - x, texCoord.y + y)).rgb;
    vec3 b = texture(srcTexture, vec2(texCoord.x,     texCoord.y + y)).rgb;
    vec3 c = texture(srcTexture, vec2(texCoord.x + x, texCoord.y + y)).rgb;

    vec3 d = texture(srcTexture, vec2(texCoord.x - x, texCoord.y)).rgb;
    vec3 e = texture(srcTexture, vec2(texCoord.x,     texCoord.y)).rgb;
    vec3 f = texture(srcTexture, vec2(texCoord.x + x, texCoord.y)).rgb;

    vec3 g = texture(srcTexture, vec2(texCoord.x - x, texCoord.y - y)).rgb;
    vec3 h = texture(srcTexture, vec2(texCoord.x,     texCoord.y - y)).rgb;
    vec3 i = texture(srcTexture, vec2(texCoord.x + x, texCoord.y - y)).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    upsample = e*4.0;
    upsample += (b+d+f+h)*2.0;
    upsample += (a+c+g+i);
    upsample *= 1.0 / 16.0;
    //upsample = min(upsample, 0.9f);
    //upsample = vec3(0.0, 0.8, 1.0);
}