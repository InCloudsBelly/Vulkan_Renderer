#include "VulkanRenderer/Extensions/extensionsUtils.h"

#include <vector>

#include "VulkanRenderer/Settings/vkLayersConfig.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

std::vector<const char*> extensionsUtils::getRequiredExtensions()
{
    std::vector<const char*> extensions;

    // - GLFW's extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i != glfwExtensionCount; i++)
        extensions.push_back(*(glfwExtensions + i));

    // - Vulkan Layers extensions
    if (vkLayersConfig::VALIDATION_LAYERS_ENABLED)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}