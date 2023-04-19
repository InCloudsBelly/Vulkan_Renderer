#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vk_mem_alloc.h>

class SkyBox
{
public:
	SkyBox();
	SkyBox(const VkRenderPass& renderPass, VkSampleCountFlagBits multisampleBits, uint32_t subPassIndex);

	~SkyBox() {};

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

	VkBuffer				m_ubo;
	VmaAllocation			m_uboAllocation;
	VkDescriptorSet			m_descriptorSet;
};

