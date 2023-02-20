#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"

struct QueueFamilyHandles
{
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    void setQueueHandles(
        const VkDevice& logicalDevice,
        const QueueFamilyIndices& qfIndices
    );
};