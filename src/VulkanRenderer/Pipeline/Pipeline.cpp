#include "VulkanRenderer/Pipeline/Pipeline.h"

#include <vector>
#include <string>
#include <stdexcept>

#include "VulkanRenderer/Shader/shaderManager.h"
#include "VulkanRenderer/Renderer.h"

Pipeline::Pipeline() {}

Pipeline::Pipeline(
    const PipelineType& type
) :m_type(type)
{}

Pipeline::~Pipeline() {}

void Pipeline::createShaderModule(const ShaderInfo& shaderInfo,VkShaderModule& shaderModule) 
{
    std::vector<char> shaderCode;

    if (shaderInfo.type == shaderType::VERTEX)
    {
        shaderCode = (ShaderManager::getBinaryDataFromFile("vert-" + shaderInfo.fileName));
    }
    else if (shaderInfo.type == shaderType::FRAGMENT)
    {
        shaderCode = (ShaderManager::getBinaryDataFromFile("frag-" + shaderInfo.fileName));
    }
    else if (shaderInfo.type == shaderType::COMPUTE)
    {
        shaderCode = (ShaderManager::getBinaryDataFromFile("comp-" + shaderInfo.fileName));
    }
    else
    {
        throw std::runtime_error("Shader type doesn't exist.");
    }

    shaderModule = ShaderManager::createShaderModule(shaderCode);
}

/*
 * Interface that creates and allows us to communicate with the uniform
 * values and push constants in the shaders.
*/
void Pipeline::createPipelineLayout(const VkDescriptorSetLayout& descriptorSetLayout, const std::vector<VkPushConstantRange>& pushConstantRanges)
{
    m_descriptorSetLayout = descriptorSetLayout; 

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // In this case we gonna bind the descriptor layout.
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    auto status = vkCreatePipelineLayout(getRendererPointer()->getDevice(),&pipelineLayoutInfo,nullptr,&m_pipelineLayout);

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");
}


const VkPipeline& Pipeline::get() const
{
    return m_pipeline;
}

const VkPipelineLayout& Pipeline::getPipelineLayout() const
{
    return m_pipelineLayout;
}

void Pipeline::destroy()
{
    vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayout, nullptr);

    vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayout, nullptr);
}

const PipelineType& Pipeline::getType() const
{
    return m_type;
}

const VkDescriptorSetLayout& Pipeline::getDescriptorSetLayout() const
{
    return m_descriptorSetLayout;
}