#version 450

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerposition;
layout (input_attachment_index = 1, binding = 2) uniform subpassInput samplerNormal;
layout (input_attachment_index = 2, binding = 3) uniform subpassInput samplerAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;
struct Light
{
    vec4    pos;
    vec4    dir;
    vec4    color;
    float   attenuation;
    float   radius;
    float   intensity;
    int     type;
};

layout(std140, binding = 0) uniform Lights
{
    Light lights[10];
};

void main() 
{
	// 从以前的子通道读取G-Buffer值
	vec3 fragPos = subpassLoad(samplerposition).rgb;
	vec3 normal = subpassLoad(samplerNormal).rgb;
	vec4 albedo = subpassLoad(samplerAlbedo);
	
//	#define ambient 0.15
//	
//	// Ambient part
//	vec3 fragcolor  = albedo.rgb * ambient;
//	
//
//	vec3 L = lights[0].pos.xyz - fragPos;
//	float dist = length(L);
//	vec3 V = ubo.viewPos.xyz - fragPos;
//	V = normalize(V);
//	L = normalize(L);
//	float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);
//
//	vec3 N = normalize(normal);
//	float NdotL = max(0.0, dot(N, L));
//	vec3 diff = ubo.lights[i].color * albedo.rgb * NdotL * atten;
//
//	// 高光贴图的值存储在反照mrt的alpha中
//	vec3 R = reflect(-L, N);
//	float NdotR = max(0.0, dot(R, V));
//	vec3 spec = ubo.lights[i].color * albedo.a * pow(NdotR, 32.0) * atten;
//
//	fragcolor += diff;	
//	
   
	outColor = albedo;
}
