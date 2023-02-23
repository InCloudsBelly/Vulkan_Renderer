#include "VulkanRenderer/Textures/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vulkan/vulkan.h>
#include <stdexcept>

#include "VulkanRenderer/Images/ImageManager.h"
#include "VulkanRenderer/Buffers/BufferManager.h"
#include "VulkanRenderer/Commands/CommandUtils.h"

Texture::Texture() {}

Texture::~Texture() {}

void Texture::transitionImageLayout(
    const VkFormat& format,
    const VkImageLayout& oldLayout,
    const VkImageLayout& newLayout,
    CommandPool& commandPool,
    VkQueue& graphicsQueue )
{
    VkCommandBuffer commandBuffer;

    commandPool.allocCommandBuffer(commandBuffer);

    commandPool.beginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,commandBuffer);

        VkImageMemoryBarrier imgMemoryBarrier{};
        imgMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        // We could use VK_IMAGE_LAYOUT_UNDEFINED if we don't care about the
        // existing contents of the image.
        imgMemoryBarrier.oldLayout = oldLayout;
        imgMemoryBarrier.newLayout = newLayout;
        // Since we are not using the barrier to transfer queue family ownership,
        // we'll ignore these two.
        imgMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemoryBarrier.image = m_textureImage;
        imgMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imgMemoryBarrier.subresourceRange.levelCount = 1;
        // In this case, our image is not an array(it has 2D coords -> texel).
        imgMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imgMemoryBarrier.subresourceRange.layerCount = 1;
        //Barriers are primarily used for synchronization purposes, so you must
        //specify which types of operations that involve the resource must happen
        //before the barrier, and which operations that involve the resource must
        //wait on the barrier. We need to do that despite already using
        //vkQueueWaitIdle to manually synchronize. The right values depend on the
        //old and new layout.
        imgMemoryBarrier.srcAccessMask = 0;
        imgMemoryBarrier.dstAccessMask = 0;

        // Defines the (pseudo)pipelines stages.
        // (this prevents that the transfer doesn't collide with the writing and
        // reading from other resources)
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
        {
            imgMemoryBarrier.srcAccessMask = 0;
            imgMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
        {
            imgMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imgMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
            throw std::invalid_argument("Unsupported layout transition!");


        CommandUtils::SYNCHRONIZATION::recordPipelineBarrier(
            sourceStage,
            destinationStage, 
            0,
            0, nullptr,
            0, nullptr,
            1, &imgMemoryBarrier,
            commandBuffer
        );

    commandPool.endCommandBuffer(commandBuffer);

    commandPool.submitCommandBuffer(graphicsQueue,commandBuffer);
}

void Texture::createTextureSampler(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice
) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // Specifies how to interpolate texels that are magnified or minified.
    // Manification -> when oversampling.
    // Minification -> when undersampling.
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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
    samplerInfo.maxLod = 0.0f;

    auto status = vkCreateSampler(logicalDevice,&samplerInfo,nullptr,&m_textureSampler);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler!");
}

void Texture::createTextureImageView(const VkDevice& logicalDevice, const VkFormat& format)  
{
    ImageManager::createImageView(logicalDevice, format, m_textureImage, VK_IMAGE_ASPECT_COLOR_BIT, m_textureImageView);
}

void Texture::createTextureImage(
    const char* pathToTexture,
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    CommandPool& commandPool,
    VkQueue& graphicsQueue
) {
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(pathToTexture, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    // *4 -> Because we are forcing the image to have RGBA.
    
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
        throw std::runtime_error("Failed to load texture image: " + std::string(pathToTexture));

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    BufferManager::createBuffer(
        physicalDevice,
        logicalDevice,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBufferMemory,
        stagingBuffer
    );

    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(logicalDevice, stagingBufferMemory);

    stbi_image_free(pixels);

    // Creates an empty Image object of determined properties.
    ImageManager::createImage(
        physicalDevice,
        logicalDevice,
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_textureImage,
        m_textureImageMemory
    );

    // We will transfer the pixels to the image object with a cmd buffer.
    // (staging buffer to the image obj)
    transitionImageLayout(
        VK_FORMAT_R8G8B8A8_SRGB,
        // Since the image was created with the VK_IMAGE_LAYOUT_UNDEFINED
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        commandPool,
        graphicsQueue
    );

    ImageManager::copyBufferToImage(
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight),
        graphicsQueue,
        commandPool,
        stagingBuffer,
        m_textureImage
    );

    // Another transition to sample from the shader.
    transitionImageLayout(
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        commandPool,
        graphicsQueue
    );

    vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}


const VkImageView& Texture::getTextureImageView() const
{
    return m_textureImageView;
}


const VkSampler& Texture::getTextureSampler() const
{
    return m_textureSampler;
}

void Texture::destroyTexture(const VkDevice& logicalDevice)
{
    vkDestroySampler(logicalDevice, m_textureSampler, nullptr);
    vkDestroyImageView(logicalDevice, m_textureImageView, nullptr);
    vkDestroyImage(logicalDevice, m_textureImage, nullptr);
    vkFreeMemory(logicalDevice, m_textureImageMemory, nullptr);
}