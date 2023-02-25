#pragma once

#include <vector>

#include "VulkanRenderer/Model/Vertex.h"
#include "VulkanRenderer/Textures/Texture.h"
#include "VulkanRenderer/Descriptors/DescriptorSets.h"
#include "VulkanRenderer/Descriptors/DescriptorTypes/UBO.h"

// REMEMBER: LoadObj automatically applies triangularization by default!

struct Model
{
	Model(const char* pathToMesh, const std::string& texture);
	
	void loadVertexInfo(const char* pathToMesh);

	void createTexture(
		const VkPhysicalDevice& physicalDevice,
		const VkDevice& logicalDevice,
		const VkFormat& format,
		CommandPool& commandPool,
		VkQueue& graphicsQueue
	);

	const VkDescriptorSet& getDescriptorSet(const uint32_t index) const;

	std::vector<Vertex>				vertices;
	std::vector<uint32_t>			indices;
	VkBuffer						vertexBuffer;
	VkDeviceMemory					vertexMemory;

	VkBuffer						indexBuffer;
	VkDeviceMemory					indexMemory;

	Texture							texture;
	std::string						textureFile;

	// Descriptors
	UBO								ubo;
	DescriptorSets					descriptorSets;
};