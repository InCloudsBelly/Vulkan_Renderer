#include "VulkanRenderer/Descriptors/Types/UBO/UBO.h"

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Buffers/bufferManager.h"
#include "VulkanRenderer/Descriptors/Types/DescriptorTypes.h"

UBO::UBO() {}

UBO::~UBO() {}

void UBO::createUniformBuffers(const VkPhysicalDevice physicalDevice,const VkDevice logicalDevice,const uint32_t nSets, const size_t size)
{
    m_uniformBuffers.resize(nSets);
    m_uniformBufferMemories.resize(nSets);

    for (size_t i = 0; i < nSets; i++)
    {
        BufferManager::createBuffer(
            physicalDevice,
            logicalDevice,
            (VkDeviceSize)size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBufferMemories[i],
            m_uniformBuffers[i]
        );
    }
}

std::vector<VkDeviceMemory>& UBO::getUniformBufferMemories()
{
    return m_uniformBufferMemories;
}

std::vector<VkBuffer>& UBO::getUniformBuffers()
{
    return m_uniformBuffers;
}

VkBuffer& UBO::getUniformBuffer(const size_t i)
{
    return m_uniformBuffers[i];
}

VkDeviceMemory& UBO::getUniformBufferMemory(const uint32_t index)
{
    return m_uniformBufferMemories[index];
}

void UBO::destroyUniformBuffersAndMemories(const VkDevice& logicalDevice)
{
    for (size_t i = 0; i < m_uniformBuffers.size(); i++)
    {
        vkDestroyBuffer(logicalDevice, m_uniformBuffers[i], nullptr);
        vkFreeMemory(logicalDevice, m_uniformBufferMemories[i], nullptr);
    }
}