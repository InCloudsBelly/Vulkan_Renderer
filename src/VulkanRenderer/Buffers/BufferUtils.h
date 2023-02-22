#pragma once

#include <vulkan/vulkan.h>

namespace BufferUtils
{
    uint32_t findMemoryType(const VkPhysicalDevice& physicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags& properties);
};