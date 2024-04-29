#VERTEX_SHADER
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoords;

uniform mat4 MVP;

out vec2 TexCoords;

void main() {
	TexCoords = texCoords;
	TexCoords.y = 1.0-texCoords.y;
	gl_Position = MVP*vec4(position.xy, 0.0, 1.0);
}

#FRAGMENT_SHADER
#version 460 core

layout(location = 0) out vec4 fragColor;
uniform vec2 iResolution;

uniform sampler2D screenTexture;
uniform int shouldTonemap;
uniform int iFrame;

in vec2 TexCoords;

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0f, 1.0f);
}

void main() {
	vec3 tColor = texture(screenTexture, TexCoords).rgb;
    
	float threshold = 5.0;
    tColor = (tColor.r > threshold && tColor.g > threshold && tColor.b > threshold) ? tColor : vec3(0.0);
	if(shouldTonemap == 1)  tColor = ACESFilm(tColor);
	fragColor = vec4(tColor, 1.0);
}