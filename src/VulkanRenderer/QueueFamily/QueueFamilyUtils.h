#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace QueueFamilyUtils
{
    bool isGraphicsQueueSupported(const VkQueueFamilyProperties& queueFamilySupported);

    bool isPresentQueueSupported(
        const int queueFamilySupportedIndex,
        const VkSurfaceKHR& surface,
        const VkPhysicalDevice& physicalDevice
    );

    void getSupportedQueueFamilies(
        const VkPhysicalDevice& physicalDevice,
        std::vector<VkQueueFamilyProperties>& queueFamilySupported
    );
};