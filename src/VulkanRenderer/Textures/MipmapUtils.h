#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Commands/CommandPool.h"

namespace MipmapUtils
{
    void generateMipmaps(
        const VkPhysicalDevice&     physicalDevice,
        CommandPool&                commandPool,
        const VkQueue&              graphicsQueue,
        const VkImage&              image,
        const int32_t               width,
        const int32_t               height,
        const VkFormat&             format,
        const int32_t               mipLevels
    );

    bool isLinearBlittingSupported(
        const VkPhysicalDevice&     physicalDevice,
        const VkFormat&             format
    );

    const int32_t getAmountOfSupportedMipLevels(
        const int32_t               width,
        const int32_t               height
    );
}