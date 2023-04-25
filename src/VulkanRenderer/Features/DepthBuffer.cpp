#include "VulkanRenderer/Features/DepthBuffer.h"

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Features/FeaturesUtils.h"

#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Renderer.h"

DepthBuffer::DepthBuffer() {}

DepthBuffer::DepthBuffer(
    const VkExtent2D& swapchainExtent,
    const VkSampleCountFlagBits& samplesCount
) {
    m_format = FeaturesUtils::findSupportedFormat(
        getRendererPointer()->getPhysicalDevice(),
        {
           VK_FORMAT_D32_SFLOAT,
           VK_FORMAT_D32_SFLOAT_S8_UINT,
           VK_FORMAT_D24_UNORM_S8_UINT
        },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );



    m_image = Image::Create2DImage(
        swapchainExtent,
        m_format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        samplesCount
    );

}

DepthBuffer::~DepthBuffer() {}

const VkImageView& DepthBuffer::getImageView() const 
{
    return m_image->getImageView();
}

const VkFormat& DepthBuffer::getFormat()const
{
    return m_format;
}

const VkImage& DepthBuffer::getImage()const
{
    return  m_image->getImage();
}

void DepthBuffer::destroy()
{
    m_image->destroy();
}