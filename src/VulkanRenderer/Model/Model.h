#pragma once

#include <vector>

#include "VulkanRenderer/Model/Vertex.h"
#include "VulkanRenderer/Textures/Texture.h"

// REMEMBER: LoadObj automatically applies triangularization by default!

struct Model
{
	void readModel(const char* pathToMesh);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexMemory;

	Texture texture;
};