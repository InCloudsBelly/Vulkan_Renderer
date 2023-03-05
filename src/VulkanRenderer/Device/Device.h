#pragma once 

#include <vulkan/vulkan.h>

#include <vector>

#include "VulkanRenderer/Queue/QueueFamilyIndices.h"
#include "VulkanRenderer/Swapchain/Swapchain.h"

class Device
{
public:
    Device(
        const VkInstance&   m_vkInstance,
        QueueFamilyIndices& requiredQueueFamilyIndices,
        const VkSurfaceKHR& windowSurface
    );
    ~Device();

    const VkDevice& getLogicalDevice() const;
    const VkPhysicalDevice& getPhysicalDevice() const;
    const SwapchainSupportedProperties& getSupportedProperties() const;


private:

    void createLogicalDevice(QueueFamilyIndices& requiredQueueFamilyIndices);

    void pickPhysicalDevice(
        const VkInstance& m_vkInstance,
        QueueFamilyIndices& requiredQueueFamilyIndices,
        const VkSurfaceKHR& windowSurface
    );

    void findSupportedProperties(
        const VkPhysicalDevice& physicalDevice,
        const VkSurfaceKHR& surface,
        SwapchainSupportedProperties& supportedProperties
    );

    bool isSwapchainAdequated(
        const VkPhysicalDevice& physicalDevice,
        const VkSurfaceKHR& surface
    );

    bool isPhysicalDeviceSuitable(
        QueueFamilyIndices& requiredQueueFamiliesIndices,
        const VkSurfaceKHR& windowSurface,
        const VkPhysicalDevice& possiblePhysicalDevice
    );
    bool areAllExtensionsSupported(
        const VkPhysicalDevice& possiblePhysicalDevice
    );

    VkPhysicalDevice               m_physicalDevice;
    VkDevice                       m_logicalDevice;
    SwapchainSupportedProperties   m_supportedProperties;

    const std::vector<const char*> m_requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};
