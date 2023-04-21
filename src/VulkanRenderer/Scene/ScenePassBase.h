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

struct ColorAttachmentInfo
{
	std::string				name;
	VkExtent2D				extent;
	VkFormat				format;
	VkSampleCountFlagBits	sampleCount;
};


class ScenePassBase
{
public:
	ScenePassBase() {};

	~ScenePassBase() {};

	virtual void updateUBO(const VkExtent2D& extent,const uint32_t& currentFrame) = 0;

	virtual void draw(uint32_t imageIndex, uint32_t frameIndex) = 0;

	const RenderPass& getRenderPass() const { return m_renderPass; }
	
	const VkFramebuffer& getSwapchainFramebuffer(uint32_t index) const { return *m_swapchain_framebuffers[index]; }

	const std::vector<VkDescriptorSet>& getMeshDescriptorSet(uint32_t meshIndex) const
	{
		auto iter = m_meshesDescriptorSetMap.find(meshIndex);
		if (iter != m_meshesDescriptorSetMap.end())
		{
			return iter->second;
		}
		else
			return {};

	}

	virtual void destroy() = 0;

private:
	virtual void createPipelines() = 0;
	virtual void createRenderPass() = 0;

	virtual void createSwapchainFramebuffers() = 0;

	virtual void createSecondaryFeatures() = 0;

	virtual void createUBOs() = 0;
	virtual void createDescriptorSets() = 0;

protected:

	void createUniformBuffer(const std::shared_ptr<Model> modelPtr, std::vector<size_t>& uboSizeInfos);
	void drawPipeline(const VkCommandBuffer& commandBuffer, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, std::vector<std::shared_ptr<Model>> models);

	void createColorAttachments(std::vector<ColorAttachmentInfo> infos);

	std::vector<VkFramebuffer*>			m_swapchain_framebuffers;

	RenderPass							m_renderPass;

	std::vector<VkClearValue>			m_clearValues;
	std::vector<VkCommandBuffer>		m_commandBuffers;

	std::vector<VkPipeline>				m_pipelines;
	std::vector <VkDescriptorSetLayout>	m_descriptorSetLayouts;
	std::vector <VkPipelineLayout>		m_pipelineLayouts;

	VkExtent2D							m_extent;

	std::shared_ptr<SkyBox>				m_skyBox;
	//GUI
	std::unique_ptr<GUI>				m_GUI;


	std::vector<std::shared_ptr<TextureBase>>	m_colorAttachments;
	std::shared_ptr<TextureBase>				m_depthAttacment;

	std::unordered_map<uint32_t, std::vector<VkBuffer>>			m_meshesUBOMap;
	std::unordered_map<uint32_t, std::vector<VmaAllocation>>	m_meshesUBOAllocationMap;
	std::unordered_map<uint32_t, std::vector<VkDescriptorSet>>	m_meshesDescriptorSetMap;

};