#pragma once

#include <vector>
#include <array>
#include <string>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Pipeline/Pipeline.h"
#include "VulkanRenderer/Descriptor/DescriptorInfo.h"

enum class GraphicsPipelineType
{
	PBR = 0,
	LIGHT = 1,
	SKYBOX = 2,
	SHADOWMAP = 3
};

class Graphics : public Pipeline
{
public:
	Graphics();
	~Graphics();

	Graphics(
		const VkDevice& logicalDevice,
		const GraphicsPipelineType type,
		const VkExtent2D& extent,
		const VkRenderPass& renderPass,
		const std::vector<ShaderInfo>& shaderInfos,
		const VkSampleCountFlagBits& samplesCount,
		VkVertexInputBindingDescription vertexBindingDescriptions,
		std::vector<VkVertexInputAttributeDescription> vertexAttribDescriptions,
		std::vector<size_t>* modelIndices,
		const std::vector<DescriptorInfo>& uboInfo,
		const std::vector<DescriptorInfo>& samplersInfo
	);


	const GraphicsPipelineType getGraphicsPipelineType() const;
	const std::vector<size_t>& getModelIndices() const;

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

	// Observer pointer
	std::vector<size_t>* m_opModelIndices;
};