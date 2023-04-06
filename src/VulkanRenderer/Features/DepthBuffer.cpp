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


    m_image = std::make_shared<NormalTexture>("Depth");
    m_image->getFormat() = m_format;
    m_image->getExtent() = VkExtent2D({ swapchainExtent.width,swapchainExtent.height });

    BufferManager::bufferCreateDepthResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(),
        m_image->getExtent(),
        m_image->getFormat(),
        samplesCount,
        &m_image->getImage(),
        &m_image->getAllocation(),
        &m_image->getImageView()
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