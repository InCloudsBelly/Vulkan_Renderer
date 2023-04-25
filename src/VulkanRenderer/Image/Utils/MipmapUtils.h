#pragma once

#include <vulkan/vulkan.h>

namespace MipmapUtils
{
    void generateMipmaps(
        const VkPhysicalDevice&     physicalDevice,
        const VkCommandPool&        commandPool,
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

    const uint32_t getAmountOfSupportedMipLevels(
        const uint32_t               width,
        const uint32_t               height
    );
}