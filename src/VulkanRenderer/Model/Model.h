#pragma once

#include <vector>

#include "VulkanRenderer/Model/Vertex.h"
#include "VulkanRenderer/Textures/Texture.h"

// REMEMBER: LoadObj automatically applies triangularization by default!

struct Model
{
	Model(const char* pathToMesh, const std::string& texture);
	
	void createTexture(
		const VkPhysicalDevice& physicalDevice,
		const VkDevice& logicalDevice,
		const VkFormat& format,
		CommandPool& commandPool,
		VkQueue& graphicsQueue
	);


	std::vector<Vertex>				vertices;
	std::vector<uint32_t>			indices;
	VkBuffer						vertexBuffer;
	VkDeviceMemory					vertexMemory;

	VkBuffer						indexBuffer;
	VkDeviceMemory					indexMemory;

	Texture							texture;

	// DescriptorType descriptorType;
	std::vector<VkDescriptorSet>	descriptorSets;
	std::string						textureFile;
};