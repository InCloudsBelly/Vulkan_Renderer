#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "VulkanRenderer/Descriptor/DescriptorManager.h"
#include "VulkanRenderer/Queue/QueueFamilyIndices.h"

class Computation
{

public:

    Computation();
    Computation(
        const std::string& shaderName,
        const uint32_t& inSize,
        const uint32_t& outSize,
        const QueueFamilyIndices& queueFamilyIndices,
        VkDescriptorPool& descriptorPool,
        const std::vector<DescriptorInfo>& bufferInfos
    );
    
    ~Computation();
    
    void execute(const VkCommandBuffer& commandBuffer);
   
    void downloadData(const uint32_t offset,void* data,const uint32_t size);

    const VkBuffer& getOutBuffer() const;
    void destroy();

private:
    VkPipeline                 m_pipeline;
    VkDescriptorSetLayout      m_descriptorSetLayout;
    VkPipelineLayout           m_pipelineLayout;


    VkDescriptorSet         m_descriptorSet;

    VkBuffer       m_inBuffer;
    VkBuffer       m_outBuffer;
    VmaAllocation  m_inAllocation;
    VmaAllocation  m_outAllocation;
};