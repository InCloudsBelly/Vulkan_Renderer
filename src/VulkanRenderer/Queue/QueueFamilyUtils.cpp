#include "VulkanRenderer/Queue/QueueFamilyUtils.h"

#include <vulkan/vulkan.h>

/*
 * Checks if the queue supported is a graphics queue.
 */
bool QueueFamilyUtils::isGraphicsQueueSupported(
    const VkQueueFamilyProperties& queueFamilySupported
) {
    return queueFamilySupported.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

/*
 * Checks if the queue supported is a compute queue.
 */
bool QueueFamilyUtils::isComputeQueueSupported(const VkQueueFamilyProperties& qfSupported) 
{
    return qfSupported.queueFlags & VK_QUEUE_COMPUTE_BIT;
}

/*
 * Checks if the Queue Family is compatible with the
 * window's surface.
 */
bool QueueFamilyUtils::isPresentQueueSupported(
    const int queueFamilySupportedIndex,
    const VkSurfaceKHR& surface,
    const VkPhysicalDevice& physicalDevice
) {
    VkBool32 isSupported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        physicalDevice,
        queueFamilySupportedIndex,
        surface,
        &isSupported
    );

    return isSupported;
}



/*
 * Gets all the queue families supported by the device with their properties.
 */
void QueueFamilyUtils::getSupportedQueueFamilies(
    const VkPhysicalDevice& physicalDevice,
    std::vector<VkQueueFamilyProperties>& queueFamilySupported
) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice,
        &queueFamilyCount,
        nullptr
    );

    queueFamilySupported.resize(queueFamilyCount);

    // Contains some details like the type of operations that are supported and
    // the number of queues that can be created.
    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice,
        &queueFamilyCount,
        queueFamilySupported.data()
    );
}