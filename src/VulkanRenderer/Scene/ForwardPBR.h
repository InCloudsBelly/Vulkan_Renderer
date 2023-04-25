#pragma once

#include "VulkanRenderer/Features/ShadowMap.h"

#include "VulkanRenderer/Scene/ScenePassBase.h"



class ForwardPBRPass : public ScenePassBase
{
public:

	enum PipelineIndex
	{
		main_pipeline = 0,
		PIPELINE_NUM
	};

	ForwardPBRPass();

	~ForwardPBRPass();

	virtual void updateUBO(const VkExtent2D& extent,const uint32_t& currentFrame) override;

	virtual void draw(uint32_t imageIndex, uint32_t frameIndex) override;

	const Computation& getComputation() const;
	const std::shared_ptr<ShadowMap> getShadowMap() const { return m_shadowMap; }

	virtual void destroy() override;

private:

	void initComputations();

	virtual void createPipelines() override;
	virtual void createRenderPass() override;

	virtual void createSwapchainFramebuffers() override;

	virtual void createSecondaryFeatures() override;

	virtual void createUBOs() override;
	virtual void createDescriptorSets() override;

	
	void loadBRDFlut();

	

	//Shadow Map
	std::shared_ptr<ShadowMap>				m_shadowMap;

	std::shared_ptr<LightSphere>			m_lightSphere;

	// IBL
	Computation								m_BRDFcomp;

	Texture									m_BRDFlut;
	std::shared_ptr<PrefilteredEnvMap>		m_prefilteredEnvMap;
	std::shared_ptr<PrefilteredIrradiance>	m_prefilteredIrradiance;

};