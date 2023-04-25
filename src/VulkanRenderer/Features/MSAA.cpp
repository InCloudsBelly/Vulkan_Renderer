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

    m_image = Image::Create2DImage(
        swapchainExtent,
        swapchainFormat,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        m_samplesCount,
        1
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