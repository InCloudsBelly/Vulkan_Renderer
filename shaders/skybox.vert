#version 450

layout(std140, binding = 0) uniform UniformBufferObject
{
   mat4 model;
   mat4 view;
   mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outTexCoord;

void main()
{
   gl_Position = (ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0)).xyww;

   outTexCoord = inPosition;
}