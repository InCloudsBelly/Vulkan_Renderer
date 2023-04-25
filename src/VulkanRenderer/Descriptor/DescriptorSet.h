#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>

class DescriptorSet
{
public:
	struct DescriptorSetWriteData
	{
		uint32_t binding;
		VkDescriptorType type;
		VkBuffer buffer;
		VkBufferView bufferView;
		VkDeviceSize offset;
		VkDeviceSize range;
		VkSampler sampler;
		VkImageView imageView;
		VkImageLayout imageLayout;
		DescriptorSetWriteData(uint32_t binding, VkDescriptorType type, VkBuffer& buffer, VkDeviceSize offset, VkDeviceSize range);
		DescriptorSetWriteData(uint32_t binding, VkDescriptorType type, VkBuffer& buffer, VkBufferView& bufferView, VkDeviceSize offset, VkDeviceSize range);
		DescriptorSetWriteData(uint32_t binding, VkDescriptorType type, VkSampler& sampler, VkImageView& imageView, VkImageLayout imageLayout);
		DescriptorSetWriteData(uint32_t binding, VkDescriptorType type, VkImageView& imageView, VkImageLayout imageLayout);
	};
	DescriptorSet() {};
	~DescriptorSet() {};

	
	VkDescriptorSet& get();
	void UpdateBindingData(std::vector<DescriptorSetWriteData> data);
private:
	VkDescriptorSet m_DescriptorSet;
	
	DescriptorSet(const DescriptorSet&) = delete;
	DescriptorSet& operator=(const DescriptorSet&) = delete;
	DescriptorSet(DescriptorSet&&) = delete;
	DescriptorSet& operator=(DescriptorSet&&) = delete;
};