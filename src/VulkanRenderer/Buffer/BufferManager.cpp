#include "VulkanRenderer/Buffer/BufferManager.h"

#include <stdexcept>
#include <cstring>

#include <stb_image.h>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Model/Attributes.h"
#include "VulkanRenderer/Buffer/BufferUtils.h"
#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/Command/CommandManager.h"

/*
 * It does:
 *    -Creates the memory for the buffer.
 *    -Allocates the buffer.
 *    -Binds the buffer with the memory.
 */

void BufferManager::createBuffer(
    const VkPhysicalDevice&     physicalDevice,
    const VkDevice&             logicalDevice,
    const VkDeviceSize          size,
    const VkBufferUsageFlags    usage,
    const VkMemoryPropertyFlags memoryProperties,
    VkDeviceMemory&             memory,
    VkBuffer&                   buffer
) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    // Indicates for which purposeS the data in the buffer is going to be
    // used.
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto status = vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer of usage: " + usage);

    allocBuffer(logicalDevice, physicalDevice, memoryProperties, buffer, memory);

    bindBufferWithMemory(logicalDevice, buffer, memory);
}


/*
 * Creates a buffer that can be shared between the graphics and compute queue.
 */
void BufferManager::createSharedConcurrentBuffer(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags memoryProperties,
    const QueueFamilyIndices& queueFamilyIndices,
    VkDeviceMemory& memory,
    VkBuffer& buffer )
{
    std::vector<uint32_t> necessaryIndices;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    // Indicates for which purposes the data in the buffer is going to be used.
    bufferInfo.usage = usage;
    bufferInfo.queueFamilyIndexCount = 2;

    // Indicates if the buffer can be owned by a specific queue family or
    // be shared between multiple at the same time.
    // .VK_SHARING_MODE_EXCLUSIVE specifies that access to any range or image
    // subresource of the object will be exclusive to a single queue family at
    // a time.
    // .VK_SHARING_MODE_CONCURRENT specifies that concurrent access to any range
    // or image subresource of the object from multiple queue families is
    // supported.

    if (queueFamilyIndices.graphicsFamily.has_value() && queueFamilyIndices.computeFamily.has_value())
    {
        necessaryIndices = { queueFamilyIndices.graphicsFamily.value(),queueFamilyIndices.computeFamily.value() };

        bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        bufferInfo.pQueueFamilyIndices = necessaryIndices.data();
    }
    else 
    {
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.pQueueFamilyIndices = nullptr;
    }

    auto status = vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer of usage: " + usage);

    allocBuffer(logicalDevice, physicalDevice, memoryProperties, buffer, memory);

    bindBufferWithMemory(logicalDevice, buffer, memory);
}


void BufferManager::bindBufferWithMemory(const VkDevice& logicalDevice, VkBuffer& buffer, VkDeviceMemory& memory)
{
    // 4 param. -> offset.
    vkBindBufferMemory(logicalDevice, buffer, memory, 0);
}

void BufferManager::copyBuffer(const std::shared_ptr<CommandPool>& commandPool, const VkDeviceSize size,
    VkBuffer& srcBuffer, VkBuffer& dstBuffer, const VkQueue& graphicsQueue)
{
    VkCommandBuffer commandBuffer;
    commandPool->allocCommandBuffer(commandBuffer, true);

    commandPool->beginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, commandBuffer);

    VkBufferCopy copyRegion{};
    // optional
    copyRegion.srcOffset = 0;
    // optional
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    CommandManager::ACTION::copyBufferToBuffer(srcBuffer, dstBuffer, 1, copyRegion, commandBuffer);

    commandPool->endCommandBuffer(commandBuffer);

    commandPool->submitCommandBuffer(graphicsQueue, { commandBuffer }, true);
}

void BufferManager::allocBuffer(const VkDevice& logicalDevice, const VkPhysicalDevice& physicalDevice,
    const VkMemoryPropertyFlags memoryProperties, VkBuffer& buffer, VkDeviceMemory& memory) 
{
    // -Memory requirements

    // VkMemoryRequirements struct:
    //    -Param. 1 -> The size of the required amount of memory in bytes.
    //    -Param. 2 -> The offset in bytes where the buffer begins in the
    //                 allocated region of memory, depends on bufferInfo
    //                 (usage and flags).
    //    -Param. 3 -> Bit field of the memory types that are suitable for the
    //                 buffer.
    //

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};

    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = BufferUtils::findMemoryType(physicalDevice,memRequirements.memoryTypeBits, memoryProperties);

    auto status = vkAllocateMemory(logicalDevice,&allocInfo,nullptr,&memory);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory!");
}


template<typename T>
void BufferManager::createBufferAndTransferToDevice(
    const std::shared_ptr<CommandPool>& commandPool,
    const VkPhysicalDevice&     physicalDevice,
    const VkDevice&             logicalDevice,
    T*                          data,
    const size_t                size,
    const VkQueue&              graphicsQueue,
    const VkBufferUsageFlags    usageDstBuffer,
    VkDeviceMemory&             memory,
    VkBuffer&                   buffer
) {

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    // Creates the staging buffer.
    createBuffer(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBufferMemory, stagingBuffer);

    fillBuffer(logicalDevice, data,0, size, stagingBufferMemory);

    // Creates the vertex buffer in the device(gpu).
    createBuffer(physicalDevice, logicalDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageDstBuffer,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory, buffer);

    copyBuffer(commandPool, size, stagingBuffer, buffer, graphicsQueue);

    destroyBuffer(logicalDevice, stagingBuffer);
    freeMemory(logicalDevice, stagingBufferMemory);
}


//////////////////////////////////Instances////////////////////////////////////

template void BufferManager::createBufferAndTransferToDevice<uint32_t>(
        const std::shared_ptr<CommandPool>& commandPool,
        const VkPhysicalDevice&     physicalDevice,
        const VkDevice&             logicalDevice,
        uint32_t*                   data,
        const size_t                size,
        const VkQueue&              graphicsQueue,
        const VkBufferUsageFlags    usageDstBuffer,
        VkDeviceMemory&             memory,
        VkBuffer&                   buffer
    );

template void BufferManager::createBufferAndTransferToDevice<Attributes::PBR::Vertex>
(
        const std::shared_ptr<CommandPool>& commandPool,
        const VkPhysicalDevice&     physicalDevice,
        const VkDevice&             logicalDevice,
        Attributes::PBR::Vertex*    data,
        const size_t                size,
        const VkQueue&              graphicsQueue,
        const VkBufferUsageFlags    usageDstBuffer,
        VkDeviceMemory&             memory,
        VkBuffer&                   buffer
    );

template void BufferManager::createBufferAndTransferToDevice< Attributes::SKYBOX::Vertex>(
        const std::shared_ptr<CommandPool>& commandPool,
        const VkPhysicalDevice&     physicalDevice,
        const VkDevice&             logicalDevice,
        Attributes::SKYBOX::Vertex* data,
        const size_t                size,
        const VkQueue&              graphicsQueue,
        const VkBufferUsageFlags    usageDstBuffer,
        VkDeviceMemory&             memory,
        VkBuffer&                   buffer
    );

template void BufferManager::createBufferAndTransferToDevice<Attributes::LIGHT::Vertex>(
        const std::shared_ptr<CommandPool>& commandPool,
        const VkPhysicalDevice&     physicalDevice,
        const VkDevice&             logicalDevice,
        Attributes::LIGHT::Vertex*  data,
        const size_t                size,
        const VkQueue&              graphicsQueue,
        const VkBufferUsageFlags    usageDstBuffer,
        VkDeviceMemory&             memory,
        VkBuffer&                   buffer
    );


///////////////////////////////////////////////////////////////////////////////



template<typename T>
void BufferManager::fillBuffer(const VkDevice& logicalDevice, T* data,const VkDeviceSize offset,const VkDeviceSize size,VkDeviceMemory& memory)
{
    void* memoryMap;
    // Allows us to access a region of the specified memory resource defined by
    // an offset and size.
    vkMapMemory(logicalDevice, memory, offset, size, 0, &memoryMap);
        std::memcpy(memoryMap, data, size);
    vkUnmapMemory(logicalDevice, memory);
}

/////////////////////////////////Instances////////////////////////////////////
template void BufferManager::fillBuffer<Attributes::PBR::Vertex>(
        const VkDevice&             logicalDevice,
        Attributes::PBR::Vertex*    data,
        const VkDeviceSize          offset,
        const VkDeviceSize          size,
        VkDeviceMemory&             memory
    );

template void BufferManager::fillBuffer<Attributes::SKYBOX::Vertex>(
        const VkDevice&             logicalDevice,
        Attributes::SKYBOX::Vertex* data,
        const VkDeviceSize          offset,
        const VkDeviceSize          size,
        VkDeviceMemory&             memory
    );

template void BufferManager::fillBuffer<uint32_t>(
        const VkDevice&             logicalDevice,
        uint32_t*                   data,
        const VkDeviceSize          offset,
        const VkDeviceSize          size,
        VkDeviceMemory&             memory
    );

template void BufferManager::fillBuffer<uint8_t>(
        const VkDevice&             logicalDevice,
        uint8_t*                    data,
        const VkDeviceSize          offset,
        const VkDeviceSize          size,
        VkDeviceMemory&             memory
    );

template void BufferManager::fillBuffer<float>(
        const VkDevice&             logicalDevice,
        float*                      data,
        const VkDeviceSize          offset,
        const VkDeviceSize          size,
        VkDeviceMemory&             memory
    );

///////////////////////////////////////////////////////////////////////////////
template<typename T>
void BufferManager::createAndFillStagingBuffer(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const VkDeviceSize size,
    const uint32_t offset,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags memoryProperties,
    VkDeviceMemory& memory,
    VkBuffer& buffer,
    T* data
) {
    BufferManager::createBuffer(physicalDevice, logicalDevice, size, usage, memoryProperties, memory, buffer);

    BufferManager::fillBuffer(logicalDevice, data, offset, size, memory);
}
////////////////////////////////////Instances//////////////////////////////////
template void BufferManager::createAndFillStagingBuffer<uint8_t>(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const VkDeviceSize size,
    const uint32_t offset,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags memoryProperties,
    VkDeviceMemory& memory,
    VkBuffer& buffer,
    uint8_t* data
    );

///////////////////////////////////////////////////////////////////////////////

void BufferManager::downloadDataFromBuffer(
    const VkDevice& logicalDevice,
    const VkDeviceSize& offset,
    const VkDeviceSize& size,
    const VkDeviceMemory& memory,
    void* outData
) {
    void* memoryMap = nullptr;

    vkMapMemory(logicalDevice, memory, offset, size, 0, &memoryMap);
        memcpy(outData, memoryMap, size);
    vkUnmapMemory(logicalDevice, memory);
}


void BufferManager::destroyBuffer(const VkDevice& logicalDevice,VkBuffer& buffer) 
{
    vkDestroyBuffer(logicalDevice, buffer, nullptr);
}

void BufferManager::freeMemory(const VkDevice& logicalDevice,VkDeviceMemory& memory) 
{
    vkFreeMemory(logicalDevice, memory, nullptr);
}