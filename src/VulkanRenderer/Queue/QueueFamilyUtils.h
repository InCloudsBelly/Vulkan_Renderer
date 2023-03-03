#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace QueueFamilyUtils
{
    bool isPresentQueueSupported(
        const int queueFamilySupportedIndex,
        const VkSurfaceKHR& surface,
        const VkPhysicalDevice& physicalDevice
    );

    bool isGraphicsQueueSupported(const VkQueueFamilyProperties& qfSupported);

    bool isComputeQueueSupported(const VkQueueFamilyProperties& qfSupported);

    void getSupportedQueueFamilies(
        const VkPhysicalDevice& physicalDevice,
        std::vector<VkQueueFamilyProperties>& queueFamilySupported
    );
};