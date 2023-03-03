#pragma once 

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Pipeline/Graphics.h"

namespace FeaturesUtils
{
	void createDepthStencilStateInfo(const GraphicsPipelineType& type,VkPipelineDepthStencilStateCreateInfo& depthStencil);

    VkFormat findSupportedFormat(const VkPhysicalDevice& phyisicalDevice,
        const std::vector<VkFormat>& candidates,
        const VkImageTiling tiling,
        const VkFormatFeatureFlags features
    );

    VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDevice& physicalDevice);
}