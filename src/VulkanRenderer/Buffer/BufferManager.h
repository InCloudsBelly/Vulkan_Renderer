#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <stb_image.h>

#include "VulkanRenderer/Buffer/BufferUtils.h"
#include "VulkanRenderer/Queue/QueueFamilyIndices.h"
#include "VulkanRenderer/Model/Attributes.h"

#include "VulkanRenderer/Texture/Texture.h"


namespace BufferManager
{
    //***********************************************************


    VkResult bufferCreateBuffer(
        VmaAllocator allocator,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage vmaUsage,
        VkBuffer* buffer,
        VmaAllocation* allocation
    );

    VkResult bufferCopyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkResult bufferCreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType viewtype,uint32_t mipmapLevel , uint32_t layerCount, VkImageAspectFlags aspectFlags, VkImageView* imageView);

    VkResult bufferCreateDepthResources(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, VkExtent2D swapChainExtent, VkFormat depthFormat, VkSampleCountFlagBits sampleFlages, VkImage* depthImage, VmaAllocation* depthImageAllocation, VkImageView* depthImageView);

    VkResult bufferCreateOffscreenResources(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, uint32_t miplevels,uint32_t arrayLayers, VkImageCreateFlags flags, VkSampleCountFlagBits sampleCounts, VkImageViewType viewType,  VmaAllocation* colorImageAllocation, std::shared_ptr<TextureBase> texture);

    VkResult createSharedConcurrentBuffer(
        VmaAllocator allocator,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        QueueFamilyIndices queueFamilyIndices,
        VmaMemoryUsage vmaUsage,
        VkBuffer* buffer,
        VmaAllocation* allocation
    );

    VkResult bufferCreateImage(
        VmaAllocator allocator,
        uint32_t width,
        uint32_t height,
        uint32_t miplevels,
        uint32_t arrayLayers,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkImageCreateFlags flags,
        VkSampleCountFlagBits sampleCounts,
        VkImage* image,
        VmaAllocation* allocation
    );

    VkResult bufferCopyBufferToImage(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t layerCount, uint32_t width, uint32_t height);

    VkResult bufferCopyBufferToImage(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, std::vector<VkBufferImageCopy>& regions, uint32_t width, uint32_t height);

    VkResult bufferCopyImageToBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageAspectFlagBits aspect, VkBuffer buffer, uint32_t layerCount, uint32_t width, uint32_t height);

    VkResult bufferCopyImageToBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkBuffer buffer, std::vector<VkBufferImageCopy>& regions, uint32_t width, uint32_t height);

    VkResult bufferTransitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageAspectFlagBits aspect, uint32_t miplevels, uint32_t layerCount, VkImageLayout oldLayout, VkImageLayout newLayout);

    VkResult bufferTransitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageAspectFlagBits aspect, uint32_t miplevels, uint32_t layerCount, VkImageLayout oldLayout, VkImageLayout newLayout);

    VkResult
        bufferCreateTextureImage(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, std::string basedir, std::string texNames, VkFormat& format, VkImageCreateFlags flags,VkImage* textureImage, VmaAllocation* textureImageAllocation, VkExtent2D* extent, uint32_t* mipmapLevel);

    VkResult
        bufferCreateTextureCubeMap(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, std::string basedir,std::string name, VkFormat& format, VkImageCreateFlags flags, VkImage* textureImage, VmaAllocation* textureImageAllocation, VkExtent2D* extent);

    //VkResult bufferCreateTexturecubeImage(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, gli::texture_cube &cube, VkImage *textureImage, VmaAllocation *textureImageAllocation, VkFormat *pformat);
    VkResult bufferCreateTextureSampler(VkDevice device,uint32_t mipmapLevel, VkFilter filter, VkSamplerAddressMode addressMode, VkSampler* textureSampler);

    VkResult bufferCreateFramebuffers(VkDevice device, std::vector<VkImageView> imageViews, std::vector<VkImageView> depthImageViews, VkRenderPass renderPass, VkExtent2D extent, std::vector<VkFramebuffer>& frameBuffers);

    VkResult bufferCreateFramebuffersOffscreen(VkDevice device, std::vector<VkImageView> positionImageViews, std::vector<VkImageView> normalImageViews, std::vector<VkImageView> albedoImageViews, std::vector<VkImageView> depthImageViews, VkRenderPass renderPass, VkExtent2D extent, std::vector<VkFramebuffer>& frameBuffers);

    VkResult bufferCopySwapChainImageToHost(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageAspectFlagBits aspect, VkImageLayout layout,
        /*gli::byte*/ unsigned char* bufferData,
        uint32_t width,
        uint32_t height,
        uint32_t imageSize);

    VkResult bufferCopyImageToHost(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageAspectFlagBits aspect, VkImageLayout layout,
        /*gli::byte*/ unsigned char* bufferData,
        uint32_t width,
        uint32_t height,
        uint32_t imageSize);

    VkResult bufferCreateVertexBuffer(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, std::vector<MeshVertex>& vertices, VkBuffer* vertexBuffer, VmaAllocation* vertexBufferAllocation);
    VkResult createBufferAndTransferToDevice(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, void* vertices, uint32_t size, VkBufferUsageFlags usageDstBuffer, VkBuffer* vertexBuffer, VmaAllocation* vertexBufferAllocation);

    VkResult
        bufferCreateIndexBuffer(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, std::vector<uint32_t>& indices, VkBuffer* indexBuffer, VmaAllocation* indexBufferAllocation);

    VkResult bufferCreateUniformBuffers(VmaAllocator
        allocator,
        uint32_t numberBuffers,
        VkDeviceSize
        bufferSize,
        std::vector<VkBuffer>& uniformBuffers,
        std::vector<VmaAllocation>& uniformBuffersAllocation);


};