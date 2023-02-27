#pragma once 

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Descriptors/DescriptorInfo.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Textures/Texture.h"

class DescriptorSets
{
public:
	void createDescriptorSets(
		const VkDevice						logicalDevice,
		const std::vector<DescriptorInfo>&	uboInfo,
		const std::vector<DescriptorInfo>&	samplersInfo,
		const std::vector<Texture>&			textures,
		std::vector<VkBuffer>&				uniformBuffers,
		const VkDescriptorSetLayout&		descriptorSetLayout,
		DescriptorPool&						descriptorPool
	);

	const VkDescriptorSet& get(const uint32_t index) const;

private:

	template<typename T>
	void createDescriptorWriteInfo(
		const T&				descriptorInfo,
		const VkDescriptorSet&	descriptorSet,
		const uint32_t&			dstBinding,
		const uint32_t&			dstArrayElement,
		const VkDescriptorType& type,
		VkWriteDescriptorSet&	descriptorWrite
	);

	std::vector<VkDescriptorSet> m_descriptorSets;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
};