#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include "VulkanRenderer/Pipeline/Compute.h"
#include "VulkanRenderer/Descriptor/DescriptorPool.h"
#include "VulkanRenderer/Descriptor/DescriptorSets.h"
#include "VulkanRenderer/Queue/QueueFamilyIndices.h"

class Computation
{

public:

    Computation();
    Computation(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const std::string& shaderName,
        const uint32_t& inSize,
        const uint32_t& outSize,
        const QueueFamilyIndices& queueFamilyIndices,
        DescriptorPool& descriptorPool,
        const std::vector<DescriptorInfo>& bufferInfos
    );
    
    ~Computation();
    
    void execute(const VkCommandBuffer& commandBuffer);
   
    void downloadData(const uint32_t offset,void* data,const uint32_t size);

    const VkBuffer& getOutBuffer() const;
    void destroy();

private:
    Compute                 m_pipeline;
    DescriptorSets          m_descriptorSet;

    VkBuffer       m_inBuffer;
    VkBuffer       m_outBuffer;
    VmaAllocation  m_inAllocation;
    VmaAllocation  m_outAllocation;
};