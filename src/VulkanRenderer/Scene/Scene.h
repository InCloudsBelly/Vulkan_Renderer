#pragma once

#include <vulkan/vulkan.h>
#include <VMa/vk_mem_alloc.h>

#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/Model/Types/Skybox.h"
#include "VulkanRenderer/Model/Types/NormalPBR.h"
#include "VulkanRenderer/Model/Types/Light.h"
#include "VulkanRenderer/Model/ModelInfo.h"

#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Settings/ComputePipelineConfig.h"
#include "VulkanRenderer/Pipeline/Graphics.h"
#include "VulkanRenderer/Computation/Computation.h"
#include "VulkanRenderer/Features/PrefilteredEnvMap.h"
#include "VulkanRenderer/Features/PreIrradiance.h"
#include "VulkanRenderer/Descriptor/DescriptorManager.h"

#include "VulkanRenderer/Texture/Texture.h"


class Scene
{
public:
	Scene();
	Scene(
		const VkFormat& format,
		const VkExtent2D& extent,
		const VkSampleCountFlagBits& msaaSamplesCount,
		const VkFormat& depthBufferFormat,
		const std::vector<ModelInfo>& modelsToLoadInfo,
		const QueueFamilyIndices& queueFamilyIndices,
		VkDescriptorPool& descriptorPoolForComputations,
		VmaAllocator& vmaAllocator
	);

	~Scene();

	void upload(
		const VkQueue& graphicsQueue,
		const VkCommandPool& commandPool,
		VkDescriptorPool& descriptorPool,
		//Features
		const std::shared_ptr<ShadowMap<Attributes::PBR::Vertex>> shadowMap
	);

	void updateUBO(
		const std::shared_ptr<Camera>& camera,
		//From the shadow map
		const glm::mat4& lightSpace,
		const VkExtent2D& extent,
		const uint32_t& currentFrame
	);

	const RenderPass& getRenderPass() const;
	const Graphics& getPBRpipeline() const;
	const Graphics& getSkyboxPipeline() const;
	const Graphics& getLightPipeline() const;
	const Computation& getComputation() const;

	void destroy();

private:

	void loadModels(const std::vector<ModelInfo>& modelsToLoadInfo);
	void loadModel(const size_t startI, const size_t chunckSize, const std::vector<ModelInfo>& modelsToLoadInfo);

	void initComputations(
		const QueueFamilyIndices& queueFamilyIndices,
		VkDescriptorPool& descriptorPoolForComputations
	);
	void loadBRDFlut(
		const VkQueue& graphicsQueue,
		const VkCommandPool& commandPool,
		const VmaAllocator& vmaAllocator
	);
	void createPipelines(const VkFormat& format, const VkExtent2D& extent, const VkSampleCountFlagBits& msaaSamplesCount);
	void createRenderPass(const VkFormat& format, const VkSampleCountFlagBits& msaaSamplesCount, const VkFormat& depthBufferFormat);


	VkDevice				m_logicalDevice;
	RenderPass				m_renderPass;
	
	Graphics				m_graphicsPipelinePBR;
	Graphics				m_graphicsPipelineSkybox;
	Graphics				m_graphicsPipelineLight;

	VmaAllocator			m_vmaAllocator;


	std::shared_ptr<Skybox>	m_skybox;
	// For now, this will be the only shadowable model of the scene.
	int                                 m_mainModelIndex;
	int                                 m_directionalLightIndex;


	// IBL
	Computation                         m_BRDFcomp;
	std::shared_ptr<NormalTexture>		m_BRDFlut;


	std::shared_ptr<PrefilteredEnvMap<Attributes::SKYBOX::Vertex>> m_prefilteredEnvMap;
	std::shared_ptr<PrefilteredIrradiance<Attributes::SKYBOX::Vertex>> m_prefilteredIrradiance;
};