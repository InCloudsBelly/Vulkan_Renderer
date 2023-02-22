#pragma once

#include <vulkan/vulkan.h>

namespace commandUtils
{
    namespace copyCommandBuffer {
        void record(const VkDeviceSize size, VkBuffer& srcBuffer,
            VkBuffer& dstBuffer, VkCommandBuffer& commandBuffer);
    };
};