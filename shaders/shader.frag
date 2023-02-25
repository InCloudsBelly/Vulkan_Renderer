#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    float lightIntensity = max(0.0,dot(inNormal, vec3(0.2, 0.2, 1.0)));
    outColor = vec4(pow(lightIntensity * texture(texSampler, inTexCoord).rgb, vec3(1.0/2.2)),1.0);
}