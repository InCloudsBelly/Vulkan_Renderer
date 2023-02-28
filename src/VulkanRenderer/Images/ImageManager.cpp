#include "VulkanRenderer/Images/ImageManager.h"

#include <stdexcept>

#include "VulkanRenderer/Commands/CommandUtils.h"
#include "VulkanRenderer/Commands/CommandPool.h"
#include "VulkanRenderer/Buffers/BufferUtils.h"

void ImageManager::createImage(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const uint32_t width,
    const uint32_t height,
    const VkFormat& format,
    const VkImageTiling& tiling,
    const VkImageUsageFlags& usage,
    const VkMemoryPropertyFlags& memoryProperties,
    const bool isCubemap,
    const uint32_t mipLevels,
    const VkSampleCountFlagBits& numSamples,
    VkImage& image,
    VkDeviceMemory& memory
) {
    // Creates an image object with the array of pixels.
    // (So later we can sample it as texels...so in 2D coords)
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    
    if (isCubemap)
    {
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.arrayLayers = 6;
    }
    else
    {
        imageInfo.flags = 0;
        imageInfo.arrayLayers = 1;
    }

    imageInfo.format = format;
    // Specifies how texels are laid out.
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // The image will be used as destination for the buffer copy, so it should
    // be set up as a transfer destination. Also, we want to be able to access
    // the image from the shader -> VK_IMAGE_USAGE_SAMPLED_BIT
    imageInfo.usage = usage;
    // Only used by one queue family(the graphics q.family because it supports
    // transfer operations).
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // Related to multisampling.
    imageInfo.samples = numSamples;

    auto status = vkCreateImage(logicalDevice, &imageInfo, nullptr, &image);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create the Image Object");


    // Allocates memory(in the device) for the image.
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = BufferUtils::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryProperties);

    status = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &memory);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate image memory!");

    // Bind the image object(it's like a buffer) to the memory.
    vkBindImageMemory(logicalDevice, image, memory, 0);
}


void ImageManager::createImageView(
    const VkDevice& logicalDevice,
    const VkFormat& format,
    const VkImage& image,
    const VkImageAspectFlags& aspectFlags,
    const bool isCubemap,
    const uint32_t mipLevels,
    VkImageView& imageView
) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    // Specifies how to treat images, as 1D textures, 2D textures, 3D
    // textures and cube maps.
   
    if (isCubemap)
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    else
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

    createInfo.format = format;
    // Specifies how we want to map all the color channels of the images
    // (E.g: map all of the channels to the red channel for a monochrome
    // texture)
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    // Specifies what the image's purpose is and which part of the image
    // should be accessed.
    // (E.g: with mipmapping leves or multiple layers)
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = mipLevels;
    createInfo.subresourceRange.baseArrayLayer = 0;
    if (isCubemap)
        createInfo.subresourceRange.layerCount = 6;
    else
        createInfo.subresourceRange.layerCount = 1;

    const auto status = vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create image views!");

}

void ImageManager::copyBufferToImage(
    const uint32_t width,
    const uint32_t height,
    const bool isCubemap,
    VkQueue& graphicsQueue,
    CommandPool& commandPool,
    VkBuffer& buffer,
    VkImage& image
) {

    VkCommandBuffer commandBuffer;

    commandPool.allocCommandBuffer(commandBuffer, true);


    commandPool.beginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, commandBuffer);
    // Specifies which part of the buffer is going to be copied to which
    // part of the image.
    VkBufferImageCopy region{};

    // Which parts of the buffer to copy.
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    // Which part of the image we want to copy the pixels(from the buffer..).
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    if (isCubemap)
        region.imageSubresource.layerCount = 6;
    else
        region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width,height,1 };

    CommandUtils::ACTION::copyBufferToImage(buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, region, commandBuffer);

    commandPool.endCommandBuffer(commandBuffer);

    commandPool.submitCommandBuffer(graphicsQueue, commandBuffer);
}
