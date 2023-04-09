#include "VulkanRenderer/Pipeline/PipelineManager.h"

#include <vector>
#include <stdexcept>

#include "VulkanRenderer/Shader/ShaderManager.h"


namespace PipelineManager
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
		const VkDescriptorSetLayout* pSetLayouts,
		uint32_t setLayoutCount)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
		pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
		return pipelineLayoutCreateInfo;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
		uint32_t setLayoutCount)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
		return pipelineLayoutCreateInfo;
	}

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo()
	{
		VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
		pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		return pipelineVertexInputStateCreateInfo;
	}

	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
		VkPrimitiveTopology topology,
		VkPipelineInputAssemblyStateCreateFlags flags,
		VkBool32 primitiveRestartEnable)
	{
		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
		pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineInputAssemblyStateCreateInfo.topology = topology;
		pipelineInputAssemblyStateCreateInfo.flags = flags;
		pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
		return pipelineInputAssemblyStateCreateInfo;
	}

	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
		VkPolygonMode polygonMode,
		VkCullModeFlags cullMode,
		VkFrontFace frontFace,
		VkPipelineRasterizationStateCreateFlags flags,
		VkBool32 depthBiasEnable,
		float depthBiasConstantFactor,
		float depthBiasSlopeFactor
	)
	{
		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
		pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
		pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
		pipelineRasterizationStateCreateInfo.cullMode = cullMode;
		pipelineRasterizationStateCreateInfo.frontFace = frontFace;
		pipelineRasterizationStateCreateInfo.flags = flags;
		pipelineRasterizationStateCreateInfo.depthBiasEnable = depthBiasEnable;
		if (depthBiasEnable == VK_TRUE)
		{
			pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = depthBiasConstantFactor;
			pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = depthBiasSlopeFactor;
		}

		return pipelineRasterizationStateCreateInfo;
	}

	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
		VkColorComponentFlags colorWriteMask,
		VkBool32 blendEnable)
	{
		VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
		pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
		pipelineColorBlendAttachmentState.blendEnable = blendEnable;
		return pipelineColorBlendAttachmentState;
	}

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
		uint32_t attachmentCount,
		const VkPipelineColorBlendAttachmentState* pAttachments)
	{
		VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
		pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
		pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
		pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
		pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
		pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
		pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
		pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional
		return pipelineColorBlendStateCreateInfo;
	}

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
		VkBool32 depthTestEnable,
		VkBool32 depthWriteEnable,
		VkCompareOp depthCompareOp)
	{
		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
		pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
		pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
		pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f;
		pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f;

		pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
		pipelineDepthStencilStateCreateInfo.front = {};
		pipelineDepthStencilStateCreateInfo.back = {};

		return pipelineDepthStencilStateCreateInfo;
	}

	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
		uint32_t viewportCount,
		uint32_t scissorCount,
		VkPipelineViewportStateCreateFlags flags)
	{
		VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
		pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pipelineViewportStateCreateInfo.viewportCount = viewportCount;
		pipelineViewportStateCreateInfo.scissorCount = scissorCount;
		pipelineViewportStateCreateInfo.flags = flags;
		return pipelineViewportStateCreateInfo;
	}

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
		VkSampleCountFlagBits rasterizationSamples,
		VkPipelineMultisampleStateCreateFlags flags)
	{
		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
		pipelineMultisampleStateCreateInfo.sType = (
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
			);
		pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
		// Optional
		pipelineMultisampleStateCreateInfo.minSampleShading = 1.0f;
		// Optional
		pipelineMultisampleStateCreateInfo.pSampleMask = nullptr;
		// Optional
		pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		// Optional
		pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

		return pipelineMultisampleStateCreateInfo;
	}

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
		const VkDynamicState* pDynamicStates,
		uint32_t dynamicStateCount,
		VkPipelineDynamicStateCreateFlags flags)
	{
		VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
		pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
		pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
		pipelineDynamicStateCreateInfo.flags = flags;
		return pipelineDynamicStateCreateInfo;
	}

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
		const std::vector<VkDynamicState> pDynamicStates,
		VkPipelineDynamicStateCreateFlags flags)
	{
		VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
		pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();
		pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
		pipelineDynamicStateCreateInfo.flags = flags;
		return pipelineDynamicStateCreateInfo;
	}

	VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
	{
		VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};
		pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
		return pipelineTessellationStateCreateInfo;
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo(
		VkPipelineLayout layout,
		VkRenderPass renderPass,
		VkPipelineCreateFlags flags)
	{
		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.layout = layout;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.flags = flags;
		pipelineCreateInfo.basePipelineIndex = -1;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		return pipelineCreateInfo;
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo()
	{
		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.basePipelineIndex = -1;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		return pipelineCreateInfo;
	}

	VkComputePipelineCreateInfo computePipelineCreateInfo(
		VkPipelineLayout layout,
		VkPipelineCreateFlags flags)
	{
		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = layout;
		computePipelineCreateInfo.flags = flags;
		return computePipelineCreateInfo;
	}

	VkPipelineVertexInputStateCreateInfo vertexInputPipelineCreateInfo(
		VkVertexInputBindingDescription* bindingDescription,
		std::vector<VkVertexInputAttributeDescription> attribDescriptions
	)
	{
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;

		vertexInputInfo.sType = (VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attribDescriptions.data();

		return vertexInputInfo;
	}





	///////////////////////////////////////////////////////////////
	//Creates the info strucutres to link the shaders to specific pipeline stages.
	void createShaderStageInfo(const VkShaderModule& shaderModule, const shaderType& type, VkPipelineShaderStageCreateInfo& shaderStageInfo)
	{
		shaderStageInfo.sType = (VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);

		if (type == shaderType::VERTEX)
			shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		else if (type == shaderType::FRAGMENT)
			shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		else if(type == shaderType::COMPUTE)
			shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		else
			throw std::runtime_error("Shader type doesn't exist");

		shaderStageInfo.module = shaderModule;
		// Entry point: Name of the function to invoke.
		shaderStageInfo.pName = "main";
		shaderStageInfo.pNext = nullptr;
		shaderStageInfo.flags = 0;
		shaderStageInfo.pSpecializationInfo = nullptr;
	}

	void createDynamicStatesInfo(const std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateInfo& dynamicStatesInfo)
	{
		dynamicStatesInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStatesInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStatesInfo.pDynamicStates = dynamicStates.data();
	}

	/*
	 * Describes the format of the vertex data that will be passed to the
	 * vertex shader.
	 */

	void createVertexShaderInputInfo(
		const VkVertexInputBindingDescription& bindingDescription,
		const std::vector<VkVertexInputAttributeDescription>& attribDescriptions,
		VkPipelineVertexInputStateCreateInfo& vertexInputInfo
	) {
		vertexInputInfo.sType = (VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
		// Bindings: Number of vertex bindings descriptions provided in
		//           pVertexBindingDescriptions
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		// Attribute descriptions: Type of the attributes passsed to the vertex
		//                         shader, which binding to load them from and at
		//                         which OFFSET.
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attribDescriptions.data();

	}

	// Describes the GEOMETRY PRIMITIVE and if the primitive restart should be
		// enabled.
		// (#) Primitive restart: Discards the most recent index values if those
		// elements formed an incomplete primitive, and restarts the primitive
		// assembly using the subsequent indices, but only assembling the immediatly
		// following element through the end of the originally specified elements.
	void createInputAssemblyInfo(
		VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo)
	{
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	}

	// Describes the region of the framebuffer that the output
	// will be renderer to. This is almost always (0,0) to (width, height).
	void createViewport(VkViewport& viewport, const VkExtent2D& extent)
	{
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)extent.width;
		viewport.height = (float)extent.height;
		// Depth values(these are the standard values)
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
	}

	void createScissor(VkRect2D& scissor, const VkExtent2D& extent)
	{
		// In this case we want to draw the entire framebuffer
		scissor.offset = { 0, 0 };
		scissor.extent = extent;
	}

	// In the case that we turned viewport and scissor as dinamically, we need to
	// specify their count at pipeline creation time.
	void createViewportStateInfo(VkPipelineViewportStateCreateInfo& viewportStateInfo)
	{
		viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateInfo.viewportCount = 1;
		viewportStateInfo.scissorCount = 1;
	}

	void createRasterizerInfo(VkPipelineRasterizationStateCreateInfo& rasterizerInfo)
	{
		rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// If depthClampEnable is set to true, then fragments that are beyond the
		// near and far planes are clamped to them as opposed to discarding them.
		// This is useful in some special cases like SHADOW MAPS.
		rasterizerInfo.depthClampEnable = VK_FALSE;
		// If rasterizerDiscardEnable is seto to true, then geometry never passes
		// through the rasterizer stage. This basically diables any output to the
		// framebuffer.
		rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
		// Determines how fragments are generated for geometry:
		//    -VK_POLYGON_MODE_FILL: fills the area of the polygon with fragments.
		//    -VK_POLYGON_MODE_LINE: polygon edges are drawn as lines.
		//    -VK_POLYGON_MODE_POINT: polygon vertices are drawn as points.
		rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerInfo.lineWidth = 1.0f;
		// Determines the type of face culling to use.

		rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;

		rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		// on a fragment's slope. Used sometimes for shadow mapping.
		/*if (m_gType == GraphicsPipelineType::SHADOWMAP)
		{
			rasterizerInfo.depthBiasEnable = VK_TRUE;
			rasterizerInfo.depthBiasConstantFactor = 4.0f;
			rasterizerInfo.depthBiasSlopeFactor = 1.5f;
		}
		else*/
		rasterizerInfo.depthBiasEnable = VK_FALSE;

		// Optional
		//rasterizerInfo.depthBiasClamp = 0.0f;
	}

	void createMultisamplingInfo(const VkSampleCountFlagBits& samplesCount, VkPipelineMultisampleStateCreateInfo& multisamplingInfo)
	{
		multisamplingInfo.sType = (
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
			);
		multisamplingInfo.sampleShadingEnable = VK_FALSE;
		multisamplingInfo.rasterizationSamples = samplesCount;
		// Optional
		multisamplingInfo.minSampleShading = 1.0f;
		// Optional
		multisamplingInfo.pSampleMask = nullptr;
		// Optional
		multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
		// Optional
		multisamplingInfo.alphaToOneEnable = VK_FALSE;
	}

	// Contains the configuration per attached framebuffer.
	// (For now, we won't use both Config.)
	void createColorBlendingAttachment(VkPipelineColorBlendAttachmentState& colorBlendAttachment)
	{
		colorBlendAttachment.colorWriteMask = (
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT
			);
		colorBlendAttachment.blendEnable = VK_FALSE;
	}

	void createColorBlendingGlobalInfo(const VkPipelineColorBlendAttachmentState& colorBlendAttachment, VkPipelineColorBlendStateCreateInfo& colorBlendingInfo)
	{
		colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingInfo.logicOpEnable = VK_FALSE;
		colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlendingInfo.attachmentCount = 1;
		colorBlendingInfo.pAttachments = &colorBlendAttachment;
		colorBlendingInfo.blendConstants[0] = 0.0f; // Optional
		colorBlendingInfo.blendConstants[1] = 0.0f; // Optional
		colorBlendingInfo.blendConstants[2] = 0.0f; // Optional
		colorBlendingInfo.blendConstants[3] = 0.0f; // Optional
	}


	void createShaderModule(const ShaderInfo& shaderInfo, VkShaderModule& shaderModule)
	{
		std::vector<char> shaderCode;

		if (shaderInfo.type == shaderType::VERTEX)
		{
			shaderCode = (ShaderManager::getBinaryDataFromFile("vert-" + shaderInfo.fileName));
		}
		else if (shaderInfo.type == shaderType::FRAGMENT)
		{
			shaderCode = (ShaderManager::getBinaryDataFromFile("frag-" + shaderInfo.fileName));
		}
		else if (shaderInfo.type == shaderType::COMPUTE)
		{
			shaderCode = (ShaderManager::getBinaryDataFromFile("comp-" + shaderInfo.fileName));
		}
		else
		{
			throw std::runtime_error("Shader type doesn't exist.");
		}

		shaderModule = ShaderManager::createShaderModule(shaderCode);
	}

}