#include "VulkanRenderer/Descriptors/DescriptorPool.h"

#include <cstring>
#include <array>
#include <stdexcept>
#include <cmath>
#include <iostream>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Descriptors/DescriptorTypes/DescriptorTypes.h"
#include "VulkanRenderer/Buffers/BufferManager.h"
#include "VulkanRenderer/Settings/config.h"

DescriptorPool::DescriptorPool() {}
DescriptorPool::~DescriptorPool() {}

void DescriptorPool::createDescriptorPool(
	const VkDevice& logicalDevice,
	const std::vector<VkDescriptorPoolSize> poolSizes,
	const uint32_t descriptorSetsCount)
{
	if (poolSizes.size() == 0)
		throw std::runtime_error("Failed to create descriptor pool!");


	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	// Specifies the maximum number of descriptor sets that may be allocated.
	poolInfo.maxSets = descriptorSetsCount;

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool!");
}

const VkDescriptorPool& DescriptorPool::getDescriptorPool() const
{
	return m_descriptorPool;
}

// Allocates all the descriptor set from all the objs.

void DescriptorPool::allocDescriptorSets(const VkDevice& logicalDevice, std::vector<VkDescriptorSet>& descriptorSets, std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) 
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSets.size());
	allocInfo.pSetLayouts = descriptorSetLayouts.data();

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptr sets!");
}

void DescriptorPool::destroyDescriptorPool(const VkDevice& logicalDevice)
{
	vkDestroyDescriptorPool(logicalDevice, m_descriptorPool, nullptr);
}
