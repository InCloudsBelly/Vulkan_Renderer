#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <VMA/vk_mem_alloc.h>

#include "VulkanRenderer/Pipeline/Compute.h"
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
    Compute                 m_pipeline;
    VkDescriptorSet         m_descriptorSet;

    VkBuffer       m_inBuffer;
    VkBuffer       m_outBuffer;
    VmaAllocation  m_inAllocation;
    VmaAllocation  m_outAllocation;
};