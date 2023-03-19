#include "VulkanRenderer/Features/MSAA.h"

#include <vulkan/vulkan.h>


#include "VulkanRenderer/Features/FeaturesUtils.h"

#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Renderer.h"


MSAA::MSAA() {}

MSAA::MSAA(
    const VkExtent2D& swapchainExtent,
    const VkFormat& swapchainFormat
) {
    m_samplesCount = FeaturesUtils::getMaxUsableSampleCount(getRendererPointer()->getPhysicalDevice());

    m_image = std::make_shared<NormalTexture>("MSAA");
    m_image->getFormat() = swapchainFormat;
    m_image->getExtent() = VkExtent2D({ swapchainExtent.width,swapchainExtent.height });

    BufferManager::bufferCreateOffscreenResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(),
        m_image->getExtent(),
        m_image->getFormat(),
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        1,
        1,
        0,
        m_samplesCount,
        VK_IMAGE_VIEW_TYPE_2D,
        &m_image->getAllocation(),
        m_image
    );
}

MSAA::~MSAA() {}


const VkSampleCountFlagBits& MSAA::getSamplesCount() const
{
    return m_samplesCount;
}

void MSAA::destroy()
{
    m_image->destroy();
}

const VkImageView& MSAA::getImageView() const
{
    return m_image->getImageView();
}