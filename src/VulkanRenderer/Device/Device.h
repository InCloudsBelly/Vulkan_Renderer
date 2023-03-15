#pragma once 

#include <vulkan/vulkan.h>

#include <vector>
#include <string>

#include "VulkanRenderer/Queue/QueueFamilyIndices.h"
#include "VulkanRenderer/Swapchain/Swapchain.h"

class Device
{
public:
    Device(
        const VkInstance&   m_vkInstance,
        QueueFamilyIndices& requiredQueueFamilyIndices,
        const VkSurfaceKHR& windowSurface,
        void* pNextChain
    );
    ~Device();

    const VkDevice& getLogicalDevice() const;
    const VkPhysicalDevice& getPhysicalDevice() const;
    const std::string& getDeviceName() const;
    const uint32_t& getApiVersion() const;
    const SwapchainSupportedProperties& getSupportedProperties() const;


private:

    void createLogicalDevice(QueueFamilyIndices& requiredQueueFamilyIndices, void* pNextChain);

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
    std::string                    m_deviceName;
    uint32_t                       m_apiVersion;
    SwapchainSupportedProperties   m_supportedProperties;

    const std::vector<const char*> m_requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME ,VK_KHR_DEVICE_GROUP_EXTENSION_NAME };
};
