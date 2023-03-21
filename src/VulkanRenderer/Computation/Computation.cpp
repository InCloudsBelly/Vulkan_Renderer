#include "VulkanRenderer/Computation/Computation.h"

#include <string>

#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Settings/ComputePipelineConfig.h"
#include "VulkanRenderer/Descriptor/DescriptorPool.h"
#include "VulkanRenderer/Descriptor/DescriptorSetLayoutManager.h"
#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Renderer.h"

Computation::Computation() {}

Computation::Computation(
    const VkPhysicalDevice& physicalDevice,
    const VkDevice& logicalDevice,
    const std::string& shaderName,
    const uint32_t& inSize,
    const uint32_t& outSize,
    const QueueFamilyIndices& queueFamilyIndices,
    DescriptorPool& descriptorPool,
    const std::vector<DescriptorInfo>& bufferInfos
) 
{
    BufferManager::createSharedConcurrentBuffer(
        getRendererPointer()->getVmaAllocator(),
        inSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        queueFamilyIndices,
        VMA_MEMORY_USAGE_CPU_ONLY,
        &m_inBuffer,
        &m_inAllocation
    );


    BufferManager::createSharedConcurrentBuffer(
        getRendererPointer()->getVmaAllocator(),
        outSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        queueFamilyIndices,
        VMA_MEMORY_USAGE_CPU_ONLY,
        &m_outBuffer,
        &m_outAllocation
    );

    // TODO: Make it custom.
    m_pipeline = Compute(logicalDevice, ShaderInfo(shaderType::COMPUTE, "BRDF"), bufferInfos, {});

    m_descriptorSet = DescriptorSets(logicalDevice, COMPUTE_PIPELINE::BRDF::BUFFERS_INFO, { m_inBuffer, m_outBuffer }, m_pipeline.getDescriptorSetLayout(), descriptorPool);

}

Computation::~Computation() {}

void Computation::execute(const VkCommandBuffer& commandBuffer)
{

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.get());

    const std::vector<VkDescriptorSet> sets = { m_descriptorSet.get(0) };
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        m_pipeline.getPipelineLayout(),
        0,
        sets.size(), sets.data(),
        0, {}
    );

    vkCmdDispatch(commandBuffer, Config::BRDF_WIDTH, Config::BRDF_HEIGHT, 1);
}

void Computation::downloadData(const uint32_t offset,void* data,const uint32_t size) 
{
    //BufferManager::downloadDataFromBuffer(logical, offset, size, m_outMemory, data);

    void* memoryMap = nullptr;
    vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_outAllocation, &memoryMap);
    memcpy(data, memoryMap, size);
    vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_outAllocation);
}

const VkBuffer& Computation::getOutBuffer() const
{
    return m_outBuffer;
}

void Computation::destroy()
{
    vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_inBuffer, m_inAllocation);
    vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_outBuffer, m_outAllocation);

    m_pipeline.destroy();
}