#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <stb_image.h>

#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/Queue/QueueFamilyIndices.h"
#include "VulkanRenderer/Model/Attributes.h"



namespace BufferManager
{
    void createBuffer(
        const VkPhysicalDevice&         physicalDevice,
        const VkDevice&                 logicalDevice,
        const VkDeviceSize              size,
        const VkBufferUsageFlags        usage,
        const VkMemoryPropertyFlags     memoryProperties,
        VkDeviceMemory&                 memory,
        VkBuffer&                       buffer
    );

    void createSharedConcurrentBuffer(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const VkDeviceSize size,
        const VkBufferUsageFlags usage,
        const VkMemoryPropertyFlags memoryProperties,
        const QueueFamilyIndices& queueFamilyIndices,
        VkDeviceMemory& memory,
        VkBuffer& buffer
    );

    template<typename T>
    void createAndFillStagingBuffer(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const VkDeviceSize size,
        const uint32_t offset,
        const VkBufferUsageFlags usage,
        const VkMemoryPropertyFlags memoryProperties,
        VkDeviceMemory& memory,
        VkBuffer& buffer,
        T* data
    );

    template<typename T>
    void createBufferAndTransferToDevice(
        const std::shared_ptr<CommandPool>&     commandPool,
        const VkPhysicalDevice&                 physicalDevice,
        const VkDevice&                         logicalDevice,
        T*                                      data,
        size_t                                  size,
        const VkQueue&                          graphicsQueue,
        const VkBufferUsageFlags                usageDstBuffer,
        VkDeviceMemory&                         memory,
        VkBuffer&                               buffer
    );

    void freeMemory(const VkDevice& logicalDevice, VkDeviceMemory& memory);
    void destroyBuffer(const VkDevice& logicalDevice, VkBuffer& buffer);

    void copyBuffer(const std::shared_ptr<CommandPool>& commandPool, const VkDeviceSize size,
        VkBuffer& srcBuffer, VkBuffer& dstBuffer, const VkQueue& graphicsQueue);

    template<typename T>
    void fillBuffer(const VkDevice& logicalDevice, T* data,const VkDeviceSize offset,const VkDeviceSize size, VkDeviceMemory& memory);

    void downloadDataFromBuffer(
        const VkDevice& logicalDevice,
        const VkDeviceSize& offset,
        const VkDeviceSize& size,
        const VkDeviceMemory& memory,
        void* outData
    );

    void allocBuffer(
        const VkDevice&             logicalDevice,
        const VkPhysicalDevice&     physicalDevice,
        const VkMemoryPropertyFlags memoryProperties,
        VkBuffer&                   buffer,
        VkDeviceMemory&             memory
    );


    void bindBufferWithMemory(
        const VkDevice&     logicalDevice,
        VkBuffer&           buffer,
        VkDeviceMemory&     memory
    );


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

    VkResult bufferCreateDepthResources(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, VkExtent2D swapChainExtent, VkFormat depthFormat, VkImage* depthImage, VmaAllocation* depthImageAllocation, VkImageView* depthImageView);

    VkResult bufferCreateOffscreenResources(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, VkExtent2D extent, VkFormat format, VkImage* image, VmaAllocation* colorImageAllocation, VkImageView* colorImageView);

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
    VkResult bufferCreateTextureSampler(VkDevice device,uint32_t mipmapLevel, VkSampler* textureSampler);

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

    VkResult bufferCreateVertexBuffer(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, std::vector<Attributes::PBR::Vertex>& vertices, VkBuffer* vertexBuffer, VmaAllocation* vertexBufferAllocation);

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