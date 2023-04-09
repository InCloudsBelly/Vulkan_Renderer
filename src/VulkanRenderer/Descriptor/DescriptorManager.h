#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "VulkanRenderer/Texture/Texture.h"

struct DescriptorInfo
{
    int bindingNumber;
    VkDescriptorType descriptorType;
    VkShaderStageFlagBits shaderStage;
};

////////////////////////////////Helper functions///////////////////////////////
inline static void createDescriptorBufferInfo(const VkBuffer& buffer, VkDescriptorBufferInfo& bufferInfo)
{
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
}

inline static void createDescriptorImageInfo(const VkImageView& imageView, const VkSampler& sampler, VkDescriptorImageInfo& imageInfo)
{
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;
}
///////////////////////////////////////////////////////////////////////////////


namespace DescriptorManager
{
    template<typename T>
    VkResult createDescriptorWriteInfo(
        const T& descriptorInfo,
        const VkDescriptorSet& descriptorSet,
        const uint32_t& dstBinding,
        const uint32_t& dstArrayElement,
        const VkDescriptorType& type,
        VkWriteDescriptorSet& descriptorWrite
    );


    VkResult createDescriptorPool(std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPool* descriptorPool);

    VkResult allocDescriptorSet(const VkDescriptorPool& pool,const VkDescriptorSetLayout& layout, VkDescriptorSet* descriptorSet);


	inline VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
	{
		VkDescriptorImageInfo descriptorImageInfo{};
		descriptorImageInfo.sampler = sampler;
		descriptorImageInfo.imageView = imageView;
		descriptorImageInfo.imageLayout = imageLayout;
		return descriptorImageInfo;
	}

	inline VkDescriptorBufferInfo descriptorBufferInfo(const VkBuffer& buffer)
	{
		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = buffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = VK_WHOLE_SIZE;
		return descriptorBufferInfo;
	}

	inline VkWriteDescriptorSet writeDescriptorSet(
		VkDescriptorSet& dstSet,
		VkDescriptorType type,
		uint32_t binding,
		VkDescriptorBufferInfo* bufferInfo,
		uint32_t descriptorCount = 1)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = dstSet;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.pBufferInfo = bufferInfo;
		writeDescriptorSet.descriptorCount = descriptorCount;
		return writeDescriptorSet;
	}

	inline VkWriteDescriptorSet writeDescriptorSet(
		VkDescriptorSet dstSet,
		VkDescriptorType type,
		uint32_t binding,
		VkDescriptorImageInfo* imageInfo,
		uint32_t descriptorCount = 1)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = dstSet;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.pImageInfo = imageInfo;
		writeDescriptorSet.descriptorCount = descriptorCount;
		return writeDescriptorSet;
	}
};
