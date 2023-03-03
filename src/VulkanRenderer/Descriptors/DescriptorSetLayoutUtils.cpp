#include "VulkanRenderer/Descriptors/DescriptorSetLayoutUtils.h"

#include <stdexcept>

#include "VulkanRenderer/Descriptors/DescriptorInfo.h"

void DescriptorSetLayoutUtils::Graphics::createDescriptorSetLayout(
	const VkDevice&						logicalDevice,
	const std::vector<DescriptorInfo>&	uboInfo,
	const std::vector<DescriptorInfo>&	samplersInfo,
	VkDescriptorSetLayout&				descriptorSetLayout )
{
	// Descriptor bindings layouts
	std::vector<VkDescriptorSetLayoutBinding> bindings(uboInfo.size() + samplersInfo.size());

	// UBOs
	for (size_t i = 0; i < uboInfo.size(); i++)
	{
		createDescriptorBindingLayout(uboInfo[i], {}, bindings[i]);
	}
	// Samplers
	for (size_t i = 0; i < samplersInfo.size(); i++)
	{
		createDescriptorBindingLayout(samplersInfo[i],{},bindings[i + uboInfo.size()]);
	}


	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	auto status = vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout);

	if (status != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");
}

/*
 * Descriptor Set layout for In/Out buffer pair.
 */
void DescriptorSetLayoutUtils::Compute::createDescriptorSetLayout(
	const VkDevice& logicalDevice,
	const std::vector<DescriptorInfo>& bufferInfos,
	VkDescriptorSetLayout& descriptorSetLayout )
{
	std::vector<VkDescriptorSetLayoutBinding> bindings(bufferInfos.size());

	for (size_t i = 0; i < bufferInfos.size(); i++)
	{
		createDescriptorBindingLayout(bufferInfos[i],{},bindings[i]);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	auto status = vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout);

	if (status != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");
}


void DescriptorSetLayoutUtils::destroyDescriptorSetLayout(
	const VkDevice&				logicalDevice,
	VkDescriptorSetLayout&		descriptorSetLayout ) 
{
	vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
}

void DescriptorSetLayoutUtils::createDescriptorBindingLayout(
	const DescriptorInfo&			descriptorInfo,
	const std::vector<VkSampler>&	immutableSamplers,
	VkDescriptorSetLayoutBinding&	layout ) 
{
	// Binding used in the shader.
	layout.binding = descriptorInfo.bindingNumber;
	layout.descriptorType = descriptorInfo.descriptorType;
	layout.descriptorCount = 1;
	layout.stageFlags = descriptorInfo.shaderStage;
	layout.pImmutableSamplers = immutableSamplers.data();
}