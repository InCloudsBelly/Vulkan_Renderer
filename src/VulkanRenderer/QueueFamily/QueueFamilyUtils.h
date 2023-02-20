#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace QueueFamilyUtils
{
    bool isGraphicsQueueSupported(const VkQueueFamilyProperties& queueFamilySupported);

    bool isPresentQueueSupported(
        const int queueFamilySupportedIndex,
        const VkSurfaceKHR& surface,
        const VkPhysicalDevice& device
    );

    void getSupportedQueueFamilies(
        const VkPhysicalDevice& device,
        std::vector<VkQueueFamilyProperties>& queueFamilySupported
    );
};