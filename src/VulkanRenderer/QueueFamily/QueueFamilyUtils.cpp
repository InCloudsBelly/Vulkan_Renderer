#include "VulkanRenderer/QueueFamily/QueueFamilyUtils.h"

#include <vulkan/vulkan.h>

/*
 * Checks if the queue supported is a graphic's queue.
 */
bool QueueFamilyUtils::isGraphicsQueueSupported(
    const VkQueueFamilyProperties& queueFamilySupported
) {
    return queueFamilySupported.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

/*
 * Checks if the Queue Family is compatible with the
 * window's surface.
 */
bool QueueFamilyUtils::isPresentQueueSupported(
    const int queueFamilySupportedIndex,
    const VkSurfaceKHR& surface,
    const VkPhysicalDevice& device
) {
    VkBool32 isSupported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        device,
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
    const VkPhysicalDevice& device,
    std::vector<VkQueueFamilyProperties>& queueFamilySupported
) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        nullptr
    );

    queueFamilySupported.resize(queueFamilyCount);

    // Contains some details like the type of operations that are supported and
    // the number of queues that can be created.
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        queueFamilySupported.data()
    );
}