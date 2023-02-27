#pragma once

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Commands/CommandPool.h"

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

    template<typename T>
    void createBufferAndTransferToDevice(
        CommandPool&                commandPool,
        const VkPhysicalDevice&     physicalDevice,
        const VkDevice&             logicalDevice,
        T*                          data,
        size_t                      size,
        VkQueue&                    graphicsQueue,
        const VkBufferUsageFlags    usageDstBuffer,
        VkDeviceMemory&             memory,
        VkBuffer&                   buffer
    );

    void freeMemory(const VkDevice& logicalDevice, VkDeviceMemory& memory);
    void destroyBuffer(const VkDevice& logicalDevice, VkBuffer& buffer);

    void copyBuffer(CommandPool& commandPool, const VkDeviceSize size, 
        VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkQueue& graphicsQueue);

    template<typename T>
    void fillBuffer(const VkDevice& logicalDevice, T* data,const VkDeviceSize offset,const VkDeviceSize size, VkDeviceMemory& memory);

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

};