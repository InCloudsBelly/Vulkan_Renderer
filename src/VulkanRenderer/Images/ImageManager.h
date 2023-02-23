#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Commands/CommandPool.h"

namespace ImageManager {

    void createImage(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const uint32_t width,
        const uint32_t height,
        const VkFormat& format,
        const VkImageTiling& tiling,
        const VkImageUsageFlags& usage,
        const VkMemoryPropertyFlags& memoryProperties,
        VkImage& image,
        VkDeviceMemory& memory
    );
    void createImageView(
        const VkDevice& logicalDevice,
        const VkFormat& format,
        const VkImage& image,
        const VkImageAspectFlags& aspectFlags,
        VkImageView& imageView
    );
    void copyBufferToImage(
        const uint32_t width,
        const uint32_t height,
        VkQueue& graphicsQueue,
        CommandPool& commandPool,
        VkBuffer& buffer,
        VkImage& image
    );
};