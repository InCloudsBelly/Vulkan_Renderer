#include "VulkanRenderer/Descriptors/DescriptorPool.h"

#include <chrono>
#include <cstring>
#include <array>
#include <stdexcept>
#include <cmath>
#include <iostream>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanRenderer/Descriptors/DescriptorTypes.h"
#include "VulkanRenderer/Descriptors/DescriptorTypeUtils.h"
#include "VulkanRenderer/Buffers/BufferManager.h"
#include "VulkanRenderer/Settings/config.h"

DescriptorPool::DescriptorPool() {}
DescriptorPool::~DescriptorPool() {}

void DescriptorPool::createDescriptorPool(const VkDevice& logicalDevice, const size_t size, const size_t maxSets)
{
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	DescriptorTypeUtils::createUboPoolSize(size, poolSizes[0]);
	DescriptorTypeUtils::createSamplerPoolSize(size, poolSizes[1]);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	// Specifies the maximum number of descriptor sets that may be allocated.
	poolInfo.maxSets = static_cast<uint32_t>(maxSets);

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool!");
}

void DescriptorPool::createUniformBuffers(const VkPhysicalDevice physicalDevice, const VkDevice logicalDevice, const size_t nSets)
{
	VkDeviceSize size = sizeof(DescriptorTypes::UniformBufferObject);

	m_uniformBuffers.resize(nSets);
	m_uniformBuffersMemory.resize(nSets);

	for (size_t i = 0; i < nSets; i++)
	{
		BufferManager::createBuffer(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffersMemory[i], m_uniformBuffers[i]);
	}
}

//Combines all the different types of descriptors set(layouts) in one layout.

void DescriptorPool::createDescriptorSetLayout(const VkDevice& logicalDevice)
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	DescriptorTypeUtils::createUboLayoutBinding(uboLayoutBinding);

	// -Combined image sampler(Layout)
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	DescriptorTypeUtils::createSamplerLayoutBinding(samplerLayoutBinding);


	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding,samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if(vkCreateDescriptorSetLayout(logicalDevice,&layoutInfo,nullptr,&m_descriptorSetLayout)!= VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");
}

void DescriptorPool::allocDescriptorSets(const VkDevice& logicalDevice)
{
	std::vector<VkDescriptorSetLayout> layouts(Config::MAX_FRAMES_IN_FLIGHT , m_descriptorSetLayout);

	m_descriptorSets.resize(Config::MAX_FRAMES_IN_FLIGHT);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_descriptorSets.size());
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) 
		throw std::runtime_error("Failed to allocate descriptr sets!");
}

void DescriptorPool::createDescriptorSets(const VkDevice logicalDevice,const VkImageView& textureImageView,const VkSampler& textureSampler) 
{
	allocDescriptorSets(logicalDevice);

	// Configures the descriptor sets
	for (size_t i = 0; i < m_descriptorSets.size(); i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		DescriptorTypeUtils::createDescriptorBufferInfo(m_uniformBuffers[i],bufferInfo);

		VkDescriptorImageInfo imageInfo{};
		DescriptorTypeUtils::createDescriptorImageInfo(textureImageView, textureSampler, imageInfo);

		// Describes how to update the descriptors.
	  // (how and which buffer/image use to bind with the each descriptor)
		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		DescriptorTypeUtils::createUboWriteInfo(bufferInfo,m_descriptorSets[i],descriptorWrites[0]);

		DescriptorTypeUtils::createSamplerWriteInfo(imageInfo,m_descriptorSets[i],descriptorWrites[1]);

		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()),descriptorWrites.data(), 0, nullptr);
	}
}

void DescriptorPool::destroyDescriptorPool(const VkDevice& logicalDevice)
{
	vkDestroyDescriptorPool(logicalDevice, m_descriptorPool, nullptr);
}

void DescriptorPool::destroyDescriptorSetLayout(const VkDevice& logicalDevice)
{
	vkDestroyDescriptorSetLayout(logicalDevice, m_descriptorSetLayout, nullptr);
}

void DescriptorPool::destroyUniformBuffersAndMemories(const VkDevice& logicalDevice) 
{
	for (size_t i = 0; i < m_uniformBuffers.size(); i++)
	{
		vkDestroyBuffer(logicalDevice,m_uniformBuffers[i],nullptr);
		vkFreeMemory(logicalDevice,m_uniformBuffersMemory[i],nullptr);
	}
}

const std::vector<VkDescriptorSet> DescriptorPool::getDescriptorSets() const
{
	return m_descriptorSets;
}

const VkDescriptorSetLayout DescriptorPool::getDescriptorSetLayout() const
{
	return m_descriptorSetLayout;
}

void DescriptorPool::updateUniformBuffer(const VkDevice& logicalDevice,const uint8_t currentFrame,const VkExtent2D extent) 
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	DescriptorTypes::UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(time * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 10.0f);

	// GLM was designed for OpenGl, where the Y coordinate of the clip coord. is
	// inverted. To compensate for that, we have to flip the sign on the scaling
	// factor of the Y axis.
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(logicalDevice, m_uniformBuffersMemory[currentFrame], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(logicalDevice, m_uniformBuffersMemory[currentFrame]);
}