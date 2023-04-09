#version 450

layout(std140, binding = 0) uniform UniformBufferObject
{
   mat4 model;
   mat4 view;
   mat4 proj;
} ubo;

layout(binding = 1) uniform sampler2D   baseColorSampler;
layout(binding = 2) uniform sampler2D   normalSampler;


layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outAlbedo;

vec3 calculateNormal();

void main()
{
   outColor = vec4(1.0f);
   outPosition = vec4(inPosition, 1.0f);
   outNormal = vec4(calculateNormal(), 1.0f);
   outAlbedo = texture(baseColorSampler, inTexCoord);
}

vec3 calculateNormal()
{
    vec3 tangentNormal = texture(normalSampler,inTexCoord).xyz ;

	vec3 q1 = dFdx(inPosition);
	vec3 q2 = dFdy(inPosition);
	vec2 st1 = dFdx(inTexCoord);
	vec2 st2 = dFdy(inTexCoord);

    vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}