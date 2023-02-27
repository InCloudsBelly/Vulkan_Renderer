#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include "VulkanRenderer/GraphicsPipeline/GraphicsPipeline.h"

namespace DepthUtils
{
    void createDepthStencilStateInfo(const GraphicsPipelineType& type, VkPipelineDepthStencilStateCreateInfo& depthStencil);

    VkFormat findSupportedFormat(
        const VkPhysicalDevice& phyisicalDevice,
        const std::vector<VkFormat>& candidates,
        const VkImageTiling tiling,
        const VkFormatFeatureFlags features
    );
};