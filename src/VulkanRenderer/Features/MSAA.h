#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Image/Image.h"

class MSAA
{

public:

    MSAA();
    MSAA(
        const VkExtent2D& swapchainExtent,
        const VkFormat& swapchainFormat
    );
    ~MSAA();

    Image* getTexture() { return m_image; }
    const VkSampleCountFlagBits& getSamplesCount() const;
    const VkImageView& getImageView() const;

    void destroy();

private:

    Image*                          m_image;
    VkSampleCountFlagBits           m_samplesCount;

};
