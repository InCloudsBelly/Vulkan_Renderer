#include "VulkanRenderer/Texture/Type/NormalTexture.h"

#include <vulkan/vulkan.h>
#include <stdexcept>

#include "VulkanRenderer/Texture/MipmapUtils.h"
#include "VulkanRenderer/Texture/Bitmap.h"
#include "VulkanRenderer/Image/ImageManager.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/Descriptor/Types/Sampler/Sampler.h"

/*
 * Creates all the texture resources.
 */
NormalTexture::NormalTexture(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const TextureToLoadInfo& textureInfo,
    const VkSampleCountFlagBits& samplesCount,
    const std::shared_ptr<CommandPool>& commandPool,
    const VkQueue& graphicsQueue,
    const UsageType& usage 
)
    :Texture(logicalDevice, TextureType::NORMAL_TEXTURE, samplesCount, textureInfo.desiredChannels, usage)
{
    VkDeviceSize imageSize;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    if (usage == UsageType::TO_COLOR)
    {
        const std::string pathToTexture = (std::string(MODEL_DIR) + textureInfo.name);

        stbi_uc* pixels = stbi_load(pathToTexture.c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);

        if (!pixels)
        {
            throw std::runtime_error("Failed to load texture image: " + std::string(pathToTexture));
        }

        m_mipLevels = MipmapUtils::getAmountOfSupportedMipLevels(m_width, m_height);

        imageSize = m_width * m_height * m_desiredChannels;

        BufferManager::createBuffer(
            physicalDevice,
            m_logicalDevice,
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBufferMemory,
            stagingBuffer
        );

        BufferManager::fillBuffer(m_logicalDevice, pixels, 0, imageSize, stagingBufferMemory);

        stbi_image_free(pixels);


        m_image = Image(
            physicalDevice,
            m_logicalDevice,
            m_width,
            m_height,
            textureInfo.format,
            VK_IMAGE_TILING_OPTIMAL,
            (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            false,
            m_mipLevels,
            m_samplesCount,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_FILTER_LINEAR
        );

        // We will transfer the pixels to the image object with a cmd buffer.
        // (staging buffer to the image obj)
        transitionImageLayout(
            textureInfo.format,
            // Since the image was created with the VK_IMAGE_LAYOUT_UNDEFINED
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            commandPool,
            graphicsQueue
        );

        ImageManager::copyBufferToImage(m_width, m_height, false, graphicsQueue, stagingBuffer, commandPool, m_image.get());

        vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);

        MipmapUtils::generateMipmaps(physicalDevice, commandPool, graphicsQueue,
            m_image.get(), m_width, m_height, textureInfo.format, m_mipLevels);
    }
    else {
        throw std::runtime_error("Unknown UsageType for texture creation");
    }
}

NormalTexture::~NormalTexture() {}