#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct UBOinfo
{
	const glm::vec4& cameraPos;
	const glm::mat4& view;
	const glm::mat4& proj;
	const glm::mat4& lightSpace;
	const uint32_t& lightsCount;
	const VkExtent2D& extent;
};

namespace BufferUtils
{
    uint32_t findMemoryType(const VkPhysicalDevice& physicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags& properties);
};