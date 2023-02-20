#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/QueueFamily/QueueFamilyUtils.h"

bool QueueFamilyIndices::AllQueueFamiliesSupported()
{
    return (
        graphicsFamily.has_value() &&
        presentFamily.has_value()
        );
}

/*
 * Checks if the queue families required are:
 * - Supported by the device.
 * - Supported by the window's surface(in the case of the "Present" qf).
 * If they do, their indices are stored.
 */
void QueueFamilyIndices::getIndicesOfRequiredQueueFamilies(
    const VkPhysicalDevice& device,
    const VkSurfaceKHR& surface
) {
    std::vector<VkQueueFamilyProperties> qfSupported;
    QueueFamilyUtils::getSupportedQueueFamilies(device, qfSupported);

    int i = 0;
    for (const auto& qf : qfSupported)
    {
        if (QueueFamilyUtils::isGraphicsQueueSupported(qf))
            graphicsFamily = i;

        if (QueueFamilyUtils::isPresentQueueSupported(i, surface, device))
            presentFamily = i;

        i++;
    }
}