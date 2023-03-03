#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include "VulkanRenderer/Pipeline/Compute.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Descriptors/DescriptorSets.h"
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
        DescriptorPool& descriptorPool
    );
    ~Computation();
    void execute(const VkCommandBuffer& commandBuffer);
    void downloadData(
        const uint32_t offset,
        void* data,
        const uint32_t size
    );
    void destroy();

private:

    VkDevice                m_logicalDevice;
    Compute                 m_pipeline;
    DescriptorSets         m_descriptorSet;
    VkDescriptorSetLayout   m_descriptorSetLayout;

    VkBuffer       m_inBuffer;
    VkBuffer       m_outBuffer;
    VkDeviceMemory m_inMemory;
    VkDeviceMemory m_outMemory;
};