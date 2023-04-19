#pragma once

#include <vulkan/vulkan.h>
#include <VMa/vk_mem_alloc.h>

#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
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


class ForwardPBRPass
{
public:
	ForwardPBRPass();

	~ForwardPBRPass();

	void updateUBO(
		const VkExtent2D& extent,
		const uint32_t& currentFrame
	);

	void draw(uint32_t imageIndex, uint32_t frameIndex);

	const RenderPass& getRenderPass() const;
	const Computation& getComputation() const;
	const VkFramebuffer& getSwapchainFramebuffer(uint32_t index) const { return *m_swapchain_framebuffers[index]; }
	const std::shared_ptr<ShadowMap> getShadowMap() const { return m_shadowMap; }

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

	void initComputations();

	void createPipelines();
	void createRenderPass();

	void createSwapchainFramebuffers();

	void createSecondaryFeatures();

	void createUBOs();
	void createDescriptorSets();

	void createUniformBuffer(const std::shared_ptr<Model> modelPtr, std::vector<size_t>& uboSizeInfos);

	void loadBRDFlut();

	void drawPipeline(const VkCommandBuffer& commandBuffer, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, std::vector<std::shared_ptr<Model>> models);

	std::vector<VkFramebuffer*>		m_swapchain_framebuffers;

	RenderPass						m_renderPass;
	
	std::vector<VkClearValue>       m_clearValues;
	std::vector<VkCommandBuffer>	m_commandBuffers;

	VkPipeline						m_pipelinePBR;
	VkDescriptorSetLayout			m_descriptorSetLayoutPBR;
	VkPipelineLayout				m_pipelineLayoutPBR;


	VkExtent2D						m_extent;

	//Shadow Map
	std::shared_ptr<ShadowMap>				m_shadowMap;

	std::shared_ptr<SkyBox>					m_skyBox;
	std::shared_ptr<LightSphere>			m_lightSphere;
	//GUI
	std::unique_ptr<GUI>					m_GUI;
	// IBL
	Computation								m_BRDFcomp;

	std::shared_ptr<NormalTexture>			m_BRDFlut;
	std::shared_ptr<PrefilteredEnvMap>		m_prefilteredEnvMap;
	std::shared_ptr<PrefilteredIrradiance>	m_prefilteredIrradiance;

	std::unordered_map<uint32_t, std::vector<VkBuffer>>			m_ubosMap;
	std::unordered_map<uint32_t, std::vector<VmaAllocation>>	m_uboAllocationsMap;
	std::unordered_map<uint32_t, std::vector<VkDescriptorSet>>	m_descriptorSetsMap;

};