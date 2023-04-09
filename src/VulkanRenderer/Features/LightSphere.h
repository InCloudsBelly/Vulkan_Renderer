#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <VMA/vk_mem_alloc.h>

class LightSphere
{
public:
	LightSphere();
	LightSphere(const VkRenderPass& renderPass, VkSampleCountFlagBits multisampleBits, uint32_t subPassIndex);

	~LightSphere() {};

	void updateUBO();
	void draw(VkCommandBuffer& commandBuffer);

	void destroy();
private:
	void createPipeline(const VkRenderPass& renderPass, VkSampleCountFlagBits multisampleBits, uint32_t subPassIndex);
	void createDescriptorSet();
	void createUBO();

	VkPipeline				m_pipeline;
	VkPipelineLayout		m_pipelineLayout;
	VkDescriptorSetLayout	m_descriptorSetLayout;

	VkDescriptorSet			m_descriptporSet;

	std::unordered_map<uint32_t, VkBuffer>			m_ubosMap;
	std::unordered_map<uint32_t, VmaAllocation>		m_uboAllocationsMap;
	std::unordered_map<uint32_t, VkDescriptorSet>	m_descriptorSetsMap;
};

