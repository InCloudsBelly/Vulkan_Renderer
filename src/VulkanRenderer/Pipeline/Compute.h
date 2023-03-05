#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Pipeline/Pipeline.h"
#include "VulkanRenderer/Descriptor/DescriptorInfo.h"

class Compute : public Pipeline
{

public:

    Compute();
    Compute(
        const VkDevice& logicalDevice,
        const ShaderInfo& shaderInfo,
        const std::vector<DescriptorInfo>& bufferInfos
    );
    ~Compute();

private:

    void createShaderStageInfo(
        const VkShaderModule& shaderModule,
        const shaderType& type,
        VkPipelineShaderStageCreateInfo& shaderStageInfo
    ) override;

    void createDescriptorSetLayout(const std::vector<DescriptorInfo>& bufferInfos);
};