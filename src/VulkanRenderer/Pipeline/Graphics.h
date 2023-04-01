#pragma once

#include <vector>
#include <array>
#include <string>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Pipeline/Pipeline.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/Descriptor/DescriptorManager.h"

enum class GraphicsPipelineType
{
	PBR = 0,
	LIGHT = 1,
	SKYBOX = 2,
	SHADOWMAP = 3,
	PREFILTER_ENV_MAP = 4
};

class Graphics : public Pipeline
{
public:
	Graphics();
	~Graphics();

	Graphics(
		const GraphicsPipelineType type,
		const VkExtent2D& extent,
		const RenderPass& renderPass,
		const std::vector<ShaderInfo>& shaderInfos,
		const VkSampleCountFlagBits& samplesCount,
		VkVertexInputBindingDescription vertexBindingDescriptions,
		std::vector<VkVertexInputAttributeDescription> vertexAttribDescriptions,
		const std::vector<DescriptorInfo>& descriptorInfo,
		const std::vector<VkPushConstantRange>& pushConstantRanges
	);


	const GraphicsPipelineType getGraphicsPipelineType() const;

private:

	void createDescriptorSetLayout(const std::vector<DescriptorInfo>& uboInfo,const std::vector<DescriptorInfo>& samplersInfo);

	void createShaderStageInfo(
		const VkShaderModule& shaderModule,
		const shaderType& type,
		VkPipelineShaderStageCreateInfo& shaderStageInfo
	)override;

	void createDynamicStatesInfo(const std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateInfo& dynamicStatesInfo);
	
	void createVertexShaderInputInfo(const VkVertexInputBindingDescription& bindingDescription,
		const std::vector<VkVertexInputAttributeDescription>& attribDescriptions,
		VkPipelineVertexInputStateCreateInfo& vertexInputInfo);

	void createInputAssemblyInfo(VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo);

	void createViewport(VkViewport& viewport, const VkExtent2D& extent);
	void createScissor(VkRect2D& scissor, const VkExtent2D& extent);
	void createViewportStateInfo(VkPipelineViewportStateCreateInfo& viewportStateInfo);
	void createRasterizerInfo(VkPipelineRasterizationStateCreateInfo& rasterizerInfo);
	void createMultisamplingInfo(const VkSampleCountFlagBits& samplesCount, VkPipelineMultisampleStateCreateInfo& multisamplingInfo);
	void createColorBlendingAttachment(VkPipelineColorBlendAttachmentState& colorBlendAttachment);
	void createColorBlendingGlobalInfo(const VkPipelineColorBlendAttachmentState& colorBlendAttachment, VkPipelineColorBlendStateCreateInfo& colorBlendingInfo);
	

	GraphicsPipelineType m_gType;
};