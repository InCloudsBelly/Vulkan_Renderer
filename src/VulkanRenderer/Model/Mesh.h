#pragma once

#include <vector>
#include <string>
#include <memory>
#include <vulkan/vulkan.h>
#include <VMA/vk_mem_alloc.h>

#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Model/Attributes.h"

template<typename T>
struct Mesh
{
	// Vertex
	std::vector<T>                         vertices;
	std::vector<uint32_t>                  indices;

	VkBuffer                               vertexBuffer;
	VkBuffer                               indexBuffer;
	VmaAllocation						   vertexAllocation;
	VmaAllocation                          indexAllocation;

	std::vector<std::shared_ptr<TextureBase>>  textures;

	std::vector<TextureToLoadInfo>         texturesToLoadInfo;


	VkDescriptorSet						   descriptorSet;
};