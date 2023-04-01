#pragma once

#include <vulkan/vulkan.h>
#include <VMa/vk_mem_alloc.h>

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
#include "VulkanRenderer/Model/ModelManager.h"

#include "VulkanRenderer/RenderDataTypes.h"
#include "VulkanRenderer/Texture/Texture.h"

#include "VulkanRenderer/Features/ShadowMap.h"


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
		const std::shared_ptr<ShadowMap<MeshVertex>> shadowMap
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
	
	const VkDescriptorSet& getMeshDescriptorSet(uint32_t meshIndex) const 
	{
		auto iter = m_descriptorSetsMap.find(meshIndex);
		if (iter != m_descriptorSetsMap.end())
		{
			return iter->second;
		}
		else
			return VkDescriptorSet();

	}

	void destroy();

private:

	void loadModels(const std::vector<ModelInfo>& modelsToLoadInfo);
	void loadModel(const uint32_t startI, const uint32_t chunckSize, const std::vector<ModelInfo>& modelsToLoadInfo);

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

	void createUniformBuffer(const std::shared_ptr<Model> modelPtr, std::vector<size_t>& uboSizeInfos);
	void createDescriptorSet(const std::shared_ptr<Model> modelPtr, const VkDescriptorPool& descriptorPool, const VkDescriptorSetLayout & layout, std::vector<VkDescriptorImageInfo*> additionalImages, const std::vector<DescriptorInfo>& descriptorInfos);

	VkDevice				m_logicalDevice;
	RenderPass				m_renderPass;
	
	Graphics				m_graphicsPipelinePBR;
	Graphics				m_graphicsPipelineSkybox;
	Graphics				m_graphicsPipelineLight;

	VmaAllocator			m_vmaAllocator;


	//std::shared_ptr<Skybox>	m_skybox;
	// For now, this will be the only shadowable model of the scene.
	int                                 m_mainModelIndex;
	int                                 m_directionalLightIndex;

	// IBL
	Computation                         m_BRDFcomp;
	std::shared_ptr<NormalTexture>		m_BRDFlut;


	std::unordered_map<uint32_t, std::vector<VkBuffer>>			m_ubosMap;
	std::unordered_map<uint32_t, std::vector<VmaAllocation>>	m_uboAllocationsMap;
	std::unordered_map<uint32_t, VkDescriptorSet>				m_descriptorSetsMap;


	std::shared_ptr<PrefilteredEnvMap> m_prefilteredEnvMap;
	std::shared_ptr<PrefilteredIrradiance> m_prefilteredIrradiance;
};