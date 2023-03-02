#pragma once

#include <vector>
#include <string>
#include <memory>

#include "VulkanRenderer/Images/Image.h"
#include "VulkanRenderer/Textures/Texture.h"
#include "VulkanRenderer/Descriptors/DescriptorSets.h"
#include "VulkanRenderer/Model/Attributes.h"

template<typename T>
struct Mesh
{
	// Vertex
	std::vector<T> m_vertices;
	std::vector<uint32_t> m_indices;

	VkBuffer       m_vertexBuffer;
	VkBuffer       m_indexBuffer;
	VkDeviceMemory m_vertexMemory;
	VkDeviceMemory m_indexMemory;

	std::vector<std::shared_ptr<Texture>> m_textures;
	std::vector<TextureToLoadInfo>        m_texturesToLoadInfo;

	// Descritors
	// (one descriptor set for all the ubo and samplers of a mesh)
	// (the same descriptor set for each frame in flight)
	DescriptorSets m_descriptorSets;

	bool m_hasTextureCoords;
};