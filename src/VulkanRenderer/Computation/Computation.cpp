#include "VulkanRenderer/Computation/Computation.h"

#include <string>
#include <stdexcept>

#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Settings/ComputePipelineConfig.h"
#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Descriptor/DescriptorManager.h"
#include "VulkanRenderer/Pipeline/PipelineManager.h"
#include "VulkanRenderer/Shader/ShaderManager.h"

#include "VulkanRenderer/Renderer.h"

Computation::Computation() {}

Computation::Computation(
    const std::string& shaderName,
    const uint32_t& inSize,
    const uint32_t& outSize,
    const QueueFamilyIndices& queueFamilyIndices,
    VkDescriptorPool& descriptorPool,
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
    //m_pipeline = Compute(ShaderInfo(shaderType::COMPUTE, "BRDF"), bufferInfos, {});

    std::vector<VkDescriptorSetLayoutBinding> bindings(bufferInfos.size());
    for (uint32_t i = 0; i < bufferInfos.size(); i++)
    {
        bindings[i].binding = bufferInfos[i].bindingNumber;
        bindings[i].descriptorType = bufferInfos[i].descriptorType;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = bufferInfos[i].shaderStage;
        bindings[i].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    auto status = vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout);
    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");

    ShaderInfo shaderInfo{ shaderType::COMPUTE, "BRDF" };

    VkShaderModule shaderModule;
    VkPipelineShaderStageCreateInfo shaderStageInfo;

    PipelineManager::createShaderModule(shaderInfo, shaderModule);
    PipelineManager::createShaderStageInfo(shaderModule, shaderInfo.type, shaderStageInfo);



    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // In this case we gonna bind the descriptor layout.
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    status = vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");

    // --------------Compute pipeline creation------------

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.basePipelineHandle = 0;
    pipelineInfo.basePipelineIndex = 0;

    status = vkCreateComputePipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline!");

    ShaderManager::destroyShaderModule(shaderModule);



    //DescriptorSet
    DescriptorManager::allocDescriptorSet(descriptorPool, m_descriptorSetLayout, &m_descriptorSet);

    VkDescriptorBufferInfo inBuffer = DescriptorManager::descriptorBufferInfo(m_inBuffer);
    VkDescriptorBufferInfo outBuffer = DescriptorManager::descriptorBufferInfo(m_outBuffer);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        DescriptorManager::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &inBuffer),
        DescriptorManager::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &outBuffer),
    };
    vkUpdateDescriptorSets(getRendererPointer()->getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

Computation::~Computation() {}

void Computation::execute(const VkCommandBuffer& commandBuffer)
{

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    const std::vector<VkDescriptorSet> sets = {m_descriptorSet};
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        m_pipelineLayout,
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

    vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayout, nullptr);
    vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayout, nullptr);
}