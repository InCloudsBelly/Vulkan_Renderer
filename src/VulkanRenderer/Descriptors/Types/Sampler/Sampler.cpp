#include "VulkanRenderer/Descriptors/Types/Sampler/Sampler.h"

#include <stdexcept>

#include <vulkan/vulkan.h>

Sampler::~Sampler() {}

Sampler::Sampler(const VkPhysicalDevice& physicalDevice, const VkDevice& logicalDevice, const uint32_t mipLevels, const VkSamplerAddressMode& addressMode, const VkFilter& filter)
    : m_logicalDevice(logicalDevice)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // Specifies how to interpolate texels that are magnified or minified.
    // Manification -> when oversampling.
    // Minification -> when undersampling.
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;

    // Limits the amount of texel samples that can be used to calculate the
    // final color.
    // (A lower value results in better performance, but lower quality results)
    // (To find the best match, we will retrieve the Phyisical Device properties)
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    // Specifies the color that is returned when sampling beyond the image with
    // clmap to border adressing mode.
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // Vk_TRUE: we can use the coords within [0, texWidth) and [0, texHeight)
    // Vk_FALSE: we can use the coords within [0, 1)
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // These two options are used in SHADOW MAPS(percentage-closer filtering).
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    // Mipmapping fields:
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    
    if (mipLevels > 1)
        samplerInfo.maxLod = (float)mipLevels;
    else
        samplerInfo.maxLod = 0.0f;

    auto status = vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &m_sampler);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler!");
}


const VkSampler& Sampler::get() const
{
    return m_sampler;
}

void Sampler::destroy()
{
    vkDestroySampler(m_logicalDevice, m_sampler, nullptr);
}