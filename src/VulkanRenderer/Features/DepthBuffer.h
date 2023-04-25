#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Image/Image.h"

class DepthBuffer
{
public:

    DepthBuffer();
    DepthBuffer(
        const VkExtent2D& swapchainExtent,
        const VkSampleCountFlagBits& samplesCount
    );
    ~DepthBuffer();

    Image* getTexture(){return m_image; }
    const VkImageView& getImageView()const;
    const VkImage& getImage()const;
    const VkFormat& getFormat()const;

    void destroy();

private:

    Image* m_image;
    VkFormat m_format;

};