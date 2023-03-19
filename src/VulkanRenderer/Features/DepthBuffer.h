#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Texture/Texture.h"

class DepthBuffer
{
public:

    DepthBuffer();
    DepthBuffer(
        const VkExtent2D& swapchainExtent,
        const VkSampleCountFlagBits& samplesCount
    );
    ~DepthBuffer();
    const VkImageView& getImageView() const;
    const VkFormat& getFormat() const;

    void destroy();

private:

    std::shared_ptr<TextureBase> m_image;
    VkFormat m_format;

};