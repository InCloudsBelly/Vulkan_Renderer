#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Pipeline/Pipeline.h"
#include "VulkanRenderer/Descriptor/DescriptorManager.h"

class Compute : public Pipeline
{

public:

    Compute();
    Compute(
        const ShaderInfo& shaderInfo,
        const std::vector<DescriptorInfo>& bufferInfos,
        const std::vector<VkPushConstantRange>& pushConstantRanges
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