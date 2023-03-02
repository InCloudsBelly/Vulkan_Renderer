#include "VulkanRenderer/Features/MSAA.h"

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Images/imageManager.h"
#include "VulkanRenderer/Images/Image.h"

#include "VulkanRenderer/Features/FeaturesUtils.h"

MSAA::MSAA() {}

MSAA::MSAA(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const VkExtent2D& swapchainExtent,
    const VkFormat& swapchainFormat
) {
    m_samplesCount = FeaturesUtils::getMaxUsableSampleCount(
        physicalDevice
    );

    m_image = Image(
        physicalDevice,
        logicalDevice,
        swapchainExtent.width,
        swapchainExtent.height,
        swapchainFormat,
        VK_IMAGE_TILING_OPTIMAL,
        (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        false,
        1,
        m_samplesCount,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    );
}

MSAA::~MSAA() {}


const VkSampleCountFlagBits& MSAA::getSamplesCount() const
{
    return m_samplesCount;
}

void MSAA::destroy()
{
    m_image.destroy();
}

const VkImageView& MSAA::getImageView() const
{
    return m_image.getImageView();
}