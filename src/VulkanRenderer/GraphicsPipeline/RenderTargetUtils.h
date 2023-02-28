#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/GraphicsPipeline/GraphicsPipeline.h"

namespace RenderTargetUtils
{
    namespace DepthBufferUtils
    {
        void createDepthStencilStateInfo(
            const GraphicsPipelineType& type,
            VkPipelineDepthStencilStateCreateInfo& depthStencil
        );

        VkFormat findSupportedFormat(
            const VkPhysicalDevice& phyisicalDevice,
            const std::vector<VkFormat>& candidates,
            const VkImageTiling tiling,
            const VkFormatFeatureFlags features
        );

    };

    namespace MSAAUtils
    {
        VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDevice& physicalDevice);
    };
};