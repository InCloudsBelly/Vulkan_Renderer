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

    std::shared_ptr<TextureBase> getTexture() { return m_image; }
    const VkSampleCountFlagBits& getSamplesCount() const;
    const VkImageView& getImageView() const;

    void destroy();

private:

    std::shared_ptr<TextureBase>    m_image;
    VkSampleCountFlagBits           m_samplesCount;

};
