#pragma once

#include <vulkan/vulkan.h>
#include <VMa/vk_mem_alloc.h>

#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Settings/ComputePipelineConfig.h"
#include "VulkanRenderer/Computation/Computation.h"
#include "VulkanRenderer/Features/PrefilteredEnvMap.h"
#include "VulkanRenderer/Features/PreIrradiance.h"
#include "VulkanRenderer/Descriptor/DescriptorManager.h"
#include "VulkanRenderer/Framebuffer/FramebufferManager.h"
#include "VulkanRenderer/Model/ModelManager.h"

#include "VulkanRenderer/RenderDataTypes.h"
#include "VulkanRenderer/Texture/Texture.h"

#include "VulkanRenderer/GUI/GUI.h"
#include "VulkanRenderer/Features/ShadowMap.h"
#include "VulkanRenderer/Features/Skybox.h"
#include "VulkanRenderer/Features/LightSphere.h"


class SHLightingPass
{
public:
	SHLightingPass();

	~SHLightingPass();

	void updateUBO(
		const VkExtent2D& extent,
		const uint32_t& currentFrame
	);

	void draw(uint32_t imageIndex, uint32_t frameIndex);

	const RenderPass& getRenderPass() const;
	const VkFramebuffer& getSwapchainFramebuffer(uint32_t index) const { return *m_swapchain_framebuffers[index]; }

	const std::vector<VkDescriptorSet>& getMeshDescriptorSet(uint32_t meshIndex) const
	{
		auto iter = m_descriptorSetsMap.find(meshIndex);
		if (iter != m_descriptorSetsMap.end())
		{
			return iter->second;
		}
		else
			return {};

	}

	void destroy();

private:
	void createPipelines();
	void createRenderPass();

	void createSwapchainFramebuffers();

	void createSecondaryFeatures();

	void createUBOs();
	void createDescriptorSets();

	void createUniformBuffer(const std::shared_ptr<Model> modelPtr, std::vector<size_t>& uboSizeInfos);

	void loadSHBRDFlut();

	void drawPipeline(const VkCommandBuffer& commandBuffer, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, std::vector<std::shared_ptr<Model>> models);

	std::vector<VkFramebuffer*>		m_swapchain_framebuffers;

	RenderPass						m_renderPass;

	std::vector<VkClearValue>       m_clearValues;
	std::vector<VkCommandBuffer>	m_commandBuffers;

	VkPipeline						m_pipeline;
	VkDescriptorSetLayout			m_descriptorSetLayout;
	VkPipelineLayout				m_pipelineLayout;


	VkExtent2D						m_extent;


	std::shared_ptr<SkyBox>					m_skyBox;
	//GUI
	std::unique_ptr<GUI>					m_GUI;

	
	std::unordered_map<uint32_t, std::vector<VkBuffer>>			m_ubosMap;
	std::unordered_map<uint32_t, std::vector<VmaAllocation>>	m_uboAllocationsMap;
	std::unordered_map<uint32_t, std::vector<VkDescriptorSet>>	m_descriptorSetsMap;

};