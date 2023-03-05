#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "VulkanRenderer/Descriptor/DescriptorInfo.h"

namespace DescriptorSetLayoutManager
{
    namespace Graphics
    {
        void createDescriptorSetLayout(
            const VkDevice& logicalDevice,
            const std::vector<DescriptorInfo>& uboInfo,
            const std::vector<DescriptorInfo>& samplersInfo,
            VkDescriptorSetLayout& descriptorSetLayout
        );

    };

    namespace Compute
    {
        void createDescriptorSetLayout(
            const VkDevice& logicalDevice,
            const std::vector<DescriptorInfo>& bufferInfos,
            VkDescriptorSetLayout& descriptorSetLayout
        );

    };

    void destroyDescriptorSetLayout(
        const VkDevice& logicalDevice,
        VkDescriptorSetLayout& descriptorSetLayout
    );

    void createDescriptorBindingLayout(
        const DescriptorInfo&           descriptorInfo,
        const std::vector<VkSampler>&   immutableSamplers,
        VkDescriptorSetLayoutBinding&   layout
    );
}