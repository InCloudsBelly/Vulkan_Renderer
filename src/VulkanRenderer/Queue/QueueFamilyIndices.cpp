#include "VulkanRenderer/Queue/QueueFamilyIndices.h"

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Queue/QueueFamilyUtils.h"

/*
 * Checks if the queue families required are:
 * - Supported by the device.
 * - Supported by the window's surface(in the case of the "Present" qf).
 * If they do, their indices are stored.
 */
void QueueFamilyIndices::getIndicesOfRequiredQueueFamilies(
    const VkPhysicalDevice& physicalDevice,
    const VkSurfaceKHR& surface
) {
    std::vector<VkQueueFamilyProperties> qfSupported;
    QueueFamilyUtils::getSupportedQueueFamilies(physicalDevice, qfSupported);

    int i = 0;
    for (const auto& qf : qfSupported)
    {
        if (QueueFamilyUtils::isGraphicsQueueSupported(qf))
            graphicsFamily = i;

        VkBool32 presentSupport = false;
        if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport) != VK_SUCCESS)
        {
            graphicsFamily = -1;
            presentFamily = -1;
            break;
        }

        if (QueueFamilyUtils::isPresentQueueSupported(i, surface, physicalDevice))
            presentFamily = i;

        if (QueueFamilyUtils::isComputeQueueSupported(qf))
            computeFamily = i;

        i++;
    }

    AllQueueFamiliesSupported = (graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value());
}