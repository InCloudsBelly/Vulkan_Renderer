#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Queue/QueueFamilyIndices.h"

struct QueueFamilyHandles
{
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;

    void setQueueHandles(
        const VkDevice& logicalDevice,
        const QueueFamilyIndices& qfIndices
    );

};