#include "VulkanRenderer/Computation/Computation.h"

#include <string>

#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Settings/ComputePipelineConfig.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Descriptors/DescriptorSetLayoutUtils.h"
#include "VulkanRenderer/Commands/CommandManager.h"
#include "VulkanRenderer/BufferManager/BufferManager.h"

Computation::Computation() {}

Computation::Computation(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const std::string& shaderName,
    const uint32_t& inSize,
    const uint32_t& outSize,
    const QueueFamilyIndices& queueFamilyIndices,
    DescriptorPool& descriptorPool
) : m_logicalDevice(logicalDevice)
{
    BufferManager::createSharedConcurrentBuffer(
        physicalDevice,
        logicalDevice,
        inSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        queueFamilyIndices,
        m_inMemory,
        m_inBuffer
    );
    BufferManager::createSharedConcurrentBuffer(
        physicalDevice,
        logicalDevice,
        outSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        queueFamilyIndices,
        m_outMemory,
        m_outBuffer
    );

    DescriptorSetLayoutUtils::Compute::createDescriptorSetLayout(m_logicalDevice, COMPUTE_PIPELINE::BRDF::BUFFERS_INFO, m_descriptorSetLayout);

    m_descriptorSet = DescriptorSets(m_logicalDevice, COMPUTE_PIPELINE::BRDF::BUFFERS_INFO, { m_inBuffer, m_outBuffer }, m_descriptorSetLayout, descriptorPool);

    m_pipeline = Compute(m_logicalDevice, m_descriptorSetLayout, { shaderType::COMPUTE,"BRDF" });
}

Computation::~Computation() {}

void Computation::execute(const VkCommandBuffer& commandBuffer)
{
    CommandManager::STATE::bindPipeline(m_pipeline.get(),PipelineType::COMPUTE,commandBuffer);
    CommandManager::STATE::bindDescriptorSets(m_pipeline.getPipelineLayout(), PipelineType::COMPUTE, 0, { m_descriptorSet.get(0) }, {}, commandBuffer);

    CommandManager::ACTION::dispatch(Config::BRDF_WIDTH, Config::BRDF_HEIGHT, 1, commandBuffer);
}

void Computation::downloadData(const uint32_t offset,void* data,const uint32_t size) 
{
    BufferManager::downloadDataFromBuffer(m_logicalDevice, offset, size, m_outMemory, data);
}

void Computation::destroy()
{
    vkDestroyBuffer(m_logicalDevice, m_inBuffer, nullptr);
    vkDestroyBuffer(m_logicalDevice, m_outBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, m_inMemory, nullptr);
    vkFreeMemory(m_logicalDevice, m_outMemory, nullptr);

    DescriptorSetLayoutUtils::destroyDescriptorSetLayout(m_logicalDevice, m_descriptorSetLayout);

    m_pipeline.destroy();
}