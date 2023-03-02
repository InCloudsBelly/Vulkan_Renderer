#include "VulkanRenderer/Features/DepthBuffer.h"

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Images/imageManager.h"
#include "VulkanRenderer/Images/Image.h"
#include "VulkanRenderer/Features/featuresUtils.h"

DepthBuffer::DepthBuffer() {}

DepthBuffer::DepthBuffer(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const VkExtent2D& swapchainExtent,
    const VkSampleCountFlagBits& samplesCount
) {
    m_format = FeaturesUtils::findSupportedFormat(
        physicalDevice,
        {
           VK_FORMAT_D32_SFLOAT,
           VK_FORMAT_D32_SFLOAT_S8_UINT,
           VK_FORMAT_D24_UNORM_S8_UINT
        },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    m_image = Image(
        physicalDevice,
        logicalDevice,
        swapchainExtent.width,
        swapchainExtent.height,
        m_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        false,
        1,
        samplesCount,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    );
}

DepthBuffer::~DepthBuffer() {}

const VkImageView& DepthBuffer::getImageView() const
{
    return m_image.getImageView();
}

const VkFormat& DepthBuffer::getFormat() const
{
    return m_format;
}

void DepthBuffer::destroy()
{
    m_image.destroy();
}