#pragma once
#include "VulkanRenderer/Scene/ScenePassBase.h"


class SHLightingPass : public ScenePassBase
{
public:
	enum PipelineIndex
	{
		main_pipeline = 0,
		PIPELINE_NUM
	};

	SHLightingPass();

	~SHLightingPass();

	virtual void updateUBO(const VkExtent2D& extent, const uint32_t& currentFrame) override;

	virtual void draw(uint32_t imageIndex, uint32_t frameIndex) override;

	virtual void destroy() override;

private:
	virtual void createPipelines() override;
	virtual void createRenderPass() override;

	virtual void createSwapchainFramebuffers() override;

	virtual void createSecondaryFeatures() override;

	virtual void createUBOs() override;
	virtual void createDescriptorSets() override;

	void loadSHBRDFlut();

	std::shared_ptr<SkyBox>					m_skyBox;
	//GUI
	std::unique_ptr<GUI>					m_GUI;
};