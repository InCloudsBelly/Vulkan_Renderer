#pragma once 

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Descriptor/DescriptorInfo.h"
#include "VulkanRenderer/Descriptor/DescriptorPool.h"
#include "VulkanRenderer/Texture/Texture.h"


struct DescriptorSetInfo
{
	const VkDescriptorImageInfo*				irradianceMap;
	const VkDescriptorImageInfo*				BRDFlutInfo;
	const VkDescriptorImageInfo*				shadowMap;
	const VkDescriptorImageInfo*				prefilteredEnvMap;

};

class DescriptorSets
{
public:
	DescriptorSets();


	DescriptorSets(
		const VkDevice logicalDevice,
		const std::vector<DescriptorInfo>& uboInfo,
		const std::vector<DescriptorInfo>& samplersInfo,
		const std::vector<std::shared_ptr<TextureBase>>& textures,
		const VkDescriptorSetLayout& descriptorSetLayout,
		DescriptorPool& descriptorPool,
		DescriptorSetInfo* additionalTextures = nullptr,
		const std::vector<VkBuffer>& UBOs = {}
	);

	DescriptorSets(
		const VkDevice						logicalDevice,
		const std::vector<DescriptorInfo>&	buffersInfo,
		const std::vector<VkBuffer>&		buffers,
		const VkDescriptorSetLayout&		descriptorSetLayout,
		DescriptorPool&						descriptorPool
	);

	DescriptorSets(const DescriptorSets& other);
	DescriptorSets& operator=(const DescriptorSets& other);

	~DescriptorSets();

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

	std::vector<VkDescriptorSet>		m_descriptorSets;
	std::vector<VkDescriptorSetLayout>	m_descriptorSetLayouts;
};