#include "VulkanRenderer/GraphicsPipeline/RenderTarget.h"

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Images/ImageManager.h"
#include "VulkanRenderer/Images/Image.h"
#include "VulkanRenderer/GraphicsPipeline/RenderTargetUtils.h"


///////////////////////////////////MSAA////////////////////////////////////////

RenderTarget::MSAA::MSAA() {}

RenderTarget::MSAA::MSAA(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const VkExtent2D& swapchainExtent,
    const VkFormat& swapchainFormat
) {
    m_samplesCount = RenderTargetUtils::MSAAUtils::getMaxUsableSampleCount(physicalDevice);

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

RenderTarget::MSAA::~MSAA() {}


const VkSampleCountFlagBits& RenderTarget::MSAA::getSamplesCount() const
{
    return m_samplesCount;
}

void RenderTarget::MSAA::destroy(const VkDevice& logicalDevice) 
{
    m_image.destroy(logicalDevice);
}

const VkImageView& RenderTarget::MSAA::getImageView() const
{
    return  m_image.getImageView();
}

/////////////////////////////////Depth buffer//////////////////////////////////

RenderTarget::DepthBuffer::DepthBuffer() {}

RenderTarget::DepthBuffer::DepthBuffer(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const VkExtent2D& swapchainExtent,
    const VkSampleCountFlagBits& samplesCount ) 
{
    m_format = RenderTargetUtils::DepthBufferUtils::findSupportedFormat(
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

RenderTarget::DepthBuffer::~DepthBuffer() {}

const VkImageView& RenderTarget::DepthBuffer::getImageView() const
{
    return m_image.getImageView();
}

const VkFormat& RenderTarget::DepthBuffer::getFormat() const
{
    return m_format;
}

void RenderTarget::DepthBuffer::destroy(const VkDevice& logicalDevice) 
{
    m_image.destroy(logicalDevice);
}