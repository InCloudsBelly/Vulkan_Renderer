#pragma once

#include <vector>
#include <string>
#include <memory>

#include "VulkanRenderer/Texture/Texture.h"

#include "VulkanRenderer/Descriptor/DescriptorSets.h"
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

	// (One descriptor set for all the ubo and samplers of a mesh)
	// (The same descriptor set for each frame in flight)
	DescriptorSets                         descriptorSets;
};