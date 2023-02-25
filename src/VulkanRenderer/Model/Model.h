#pragma once

#include <vector>
#include <array>
#include <string>

#include <glm/glm.hpp>

#include "VulkanRenderer/Model/Vertex.h"
#include "VulkanRenderer/Textures/Texture.h"
#include "VulkanRenderer/Descriptors/DescriptorSets.h"
#include "VulkanRenderer/Descriptors/DescriptorTypes/UBO.h"

// REMEMBER: LoadObj automatically applies triangularization by default!

struct Model
{
	Model(const char* pathToMesh, const std::string& texture, const std::string& nameMesh);
	
	void loadVertexInfo(const char* pathToMesh);

	void createTexture(
		const VkPhysicalDevice& physicalDevice,
		const VkDevice& logicalDevice,
		const VkFormat& format,
		CommandPool& commandPool,
		VkQueue& graphicsQueue
	);
	void translateToCenter();
	void initExtremeValues();
	void makeItSmaller(const float maxZ);

	const VkDescriptorSet& getDescriptorSet(const uint32_t index) const;

	std::string name;

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

	// Info to update UBO.
	float							extremeX[2];
	float							extremeY[2];
	float							extremeZ[2];
	glm::fvec3						actualPos;
	glm::fvec3						actualSize;
	glm::fvec3						actualRot;
};