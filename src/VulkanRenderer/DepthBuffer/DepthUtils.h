#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace DepthUtils
{
    void createDepthStencilStateInfo(VkPipelineDepthStencilStateCreateInfo& depthStencil);

    VkFormat findSupportedFormat(
        const VkPhysicalDevice& phyisicalDevice,
        const std::vector<VkFormat>& candidates,
        const VkImageTiling tiling,
        const VkFormatFeatureFlags features
    );
};