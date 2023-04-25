#pragma once

#include "VulkanRenderer/Features/ShadowMap.h"

#include "VulkanRenderer/Scene/ScenePassBase.h"

class DeferredRenderPass : public ScenePassBase
{
public:
	enum PipelineEnum
	{
		scene_gbuffer = 0,
		composition,
		PIPELINE_NUM
	};

	enum AttachmentEnum
	{
		position = 0,
		normal,
		albedo,
		metallic_roughness,
		emissive,
		ao,
		depth,
		ATTACHMENT_NUM
	};

	DeferredRenderPass();

	~DeferredRenderPass();

	virtual void updateUBO(const VkExtent2D& extent, const uint32_t& currentFrame) override;

	virtual void draw(uint32_t imageIndex, uint32_t frameIndex) override;


	const std::shared_ptr<ShadowMap> getShadowMap() const { return m_shadowMap; }

	virtual void destroy() override;

private:
	virtual void createPipelines() override;
	virtual void createRenderPass() override;

	virtual void createSwapchainFramebuffers() override;

	virtual void createSecondaryFeatures() override;

	virtual void createUBOs() override;
	virtual void createDescriptorSets() override;



	//Shadow Map
	std::shared_ptr<ShadowMap>				m_shadowMap;

	std::shared_ptr<LightSphere>			m_lightSphere;

	Texture									m_BRDFlut;
	std::shared_ptr<PrefilteredEnvMap>		m_prefilteredEnvMap;
	std::shared_ptr<PrefilteredIrradiance>	m_prefilteredIrradiance;


	// composition
	std::vector <VkBuffer>					m_compositionUBO;
	std::vector <VmaAllocation>				m_compositionUBOAllocation;
	DescriptorSet							m_compositionDescriptorSet;
};