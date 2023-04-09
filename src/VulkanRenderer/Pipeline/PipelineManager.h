#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

enum shaderType
{
    NONE = 0,
    VERTEX = 1,
    FRAGMENT = 2,
    COMPUTE = 3
};

struct ShaderInfo
{
	shaderType type;
	std::string fileName;
	
	ShaderInfo(const shaderType& sType, const std::string& fName)
	    : type(sType), fileName(fName) {}
};


namespace PipelineManager
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(const VkDescriptorSetLayout* pSetLayouts,uint32_t setLayoutCount = 1);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(uint32_t setLayoutCount = 1);

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo();

	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
		VkPrimitiveTopology topology,
		VkPipelineInputAssemblyStateCreateFlags flags,
		VkBool32 primitiveRestartEnable);

	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
		VkPolygonMode polygonMode,
		VkCullModeFlags cullMode,
		VkFrontFace frontFace,
		VkPipelineRasterizationStateCreateFlags flags = 0,
		VkBool32 depthBiasEnable = VK_FALSE,
		float depthBiasConstantFactor = 4.0f,
		float depthBiasSlopeFactor = 1.5f
	);

	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
		VkColorComponentFlags colorWriteMask,
		VkBool32 blendEnable);

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
		uint32_t attachmentCount,
		const VkPipelineColorBlendAttachmentState* pAttachments);

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
		VkBool32 depthTestEnable,
		VkBool32 depthWriteEnable,
		VkCompareOp depthCompareOp);

	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
		uint32_t viewportCount,
		uint32_t scissorCount,
		VkPipelineViewportStateCreateFlags flags = 0);

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(VkSampleCountFlagBits rasterizationSamples,VkPipelineMultisampleStateCreateFlags flags = 0);

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(const VkDynamicState* pDynamicStates,uint32_t dynamicStateCount,VkPipelineDynamicStateCreateFlags flags = 0);

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(const std::vector<VkDynamicState> pDynamicStates,VkPipelineDynamicStateCreateFlags flags = 0);

	VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(uint32_t patchControlPoints);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo(VkPipelineLayout layout,VkRenderPass renderPass,VkPipelineCreateFlags flags = 0);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo();

	VkComputePipelineCreateInfo computePipelineCreateInfo(VkPipelineLayout layout,VkPipelineCreateFlags flags = 0);

	VkPipelineVertexInputStateCreateInfo vertexInputPipelineCreateInfo(VkVertexInputBindingDescription* bindingDescription,std::vector<VkVertexInputAttributeDescription> attribDescriptions);

	void createShaderStageInfo(const VkShaderModule& shaderModule, const shaderType& type, VkPipelineShaderStageCreateInfo& shaderStageInfo);

	void createDynamicStatesInfo(const std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateInfo& dynamicStatesInfo);

	void createVertexShaderInputInfo(
		const VkVertexInputBindingDescription& bindingDescription,
		const std::vector<VkVertexInputAttributeDescription>& attribDescriptions,
		VkPipelineVertexInputStateCreateInfo& vertexInputInfo
	);

	void createInputAssemblyInfo(VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo);

	void createViewport(VkViewport& viewport, const VkExtent2D& extent);

	void createScissor(VkRect2D& scissor, const VkExtent2D& extent);

	void createViewportStateInfo(VkPipelineViewportStateCreateInfo& viewportStateInfo);

	void createRasterizerInfo(VkPipelineRasterizationStateCreateInfo& rasterizerInfo);

	void createMultisamplingInfo(const VkSampleCountFlagBits& samplesCount, VkPipelineMultisampleStateCreateInfo& multisamplingInfo);

	void createColorBlendingAttachment(VkPipelineColorBlendAttachmentState& colorBlendAttachment);

	void createColorBlendingGlobalInfo(const VkPipelineColorBlendAttachmentState& colorBlendAttachment, VkPipelineColorBlendStateCreateInfo& colorBlendingInfo);

	void createShaderModule(const ShaderInfo& shaderInfo, VkShaderModule& shaderModule);

	
};