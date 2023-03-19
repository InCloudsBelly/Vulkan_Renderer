#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Texture/Texture.h"

class MSAA
{

public:

    MSAA();
    MSAA(
        const VkExtent2D& swapchainExtent,
        const VkFormat& swapchainFormat
    );
    ~MSAA();

    const VkSampleCountFlagBits& getSamplesCount() const;
    const VkImageView& getImageView() const;

    void destroy();

private:

    std::shared_ptr<TextureBase>    m_image;
    VkSampleCountFlagBits           m_samplesCount;

};
