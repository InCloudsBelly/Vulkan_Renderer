#include "Skybox.h"

#include <stdexcept>

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Pipeline/PipelineManager.h"
#include "VulkanRenderer/Shader/ShaderManager.h"
#include "VulkanRenderer/Math/MathUtils.h"

SkyBox::SkyBox(const VkRenderPass& renderPass, VkSampleCountFlagBits multisampleBits, uint32_t subPassIndex)
{
	createPipeline(renderPass, multisampleBits, subPassIndex);
    createUBO();
    createDescriptorSet();
}

void SkyBox::createPipeline(const VkRenderPass& renderPass, VkSampleCountFlagBits multisampleBits, uint32_t subPassIndex)
{
    const std::vector<DescriptorInfo>& descriptorInfo = GRAPHICS_PIPELINE::SKYBOX::DESCRIPTORS_INFO;

    std::vector<VkDescriptorSetLayoutBinding> bindings(descriptorInfo.size());
    for (uint32_t i = 0; i < descriptorInfo.size(); i++)
    {
        bindings[i].binding = descriptorInfo[i].bindingNumber;
        bindings[i].descriptorType = descriptorInfo[i].descriptorType;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = descriptorInfo[i].shaderStage;
        bindings[i].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");


    // -------------------Shader Modules--------------------
    const std::vector<ShaderInfo>& shaderInfos = { {shaderType::VERTEX, "skybox"}, {shaderType::FRAGMENT, "skybox"} };

    std::vector<VkShaderModule> shaderModules(shaderInfos.size());
    std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos(shaderInfos.size());
    for (uint32_t i = 0; i < shaderInfos.size(); i++)
    {
        PipelineManager::createShaderModule(shaderInfos[i], shaderModules[i]);
        PipelineManager::createShaderStageInfo(shaderModules[i], shaderInfos[i].type, shaderStagesInfos[i]);
    }


    // Dynamic states
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = PipelineManager::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());

 
    // Viewport state info
    VkPipelineViewportStateCreateInfo viewportState = PipelineManager::pipelineViewportStateCreateInfo(1, 1, 0);
    // Vertex input(attributes)
    std::vector<VkVertexInputAttributeDescription> attribDescription = Attributes::SKYBOX::getAttributeDescriptions();
    VkVertexInputBindingDescription bindingDescription = Attributes::SKYBOX::getBindingDescription();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    PipelineManager::createVertexShaderInputInfo(bindingDescription, attribDescription, vertexInputInfo);
    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = PipelineManager::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizationState = PipelineManager::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0, VK_FALSE);
    // Multisampling 
    VkPipelineMultisampleStateCreateInfo multisampleState = PipelineManager::pipelineMultisampleStateCreateInfo(multisampleBits, 0);
    // Color blending(attachment)
    VkPipelineColorBlendAttachmentState blendAttachmentState = PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

    // Color blending(global)
    VkPipelineColorBlendStateCreateInfo colorBlendState = PipelineManager::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    auto status = vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");

    // Depth and stencil
    VkPipelineDepthStencilStateCreateInfo depthStencilState = PipelineManager::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderInfos.size();
    pipelineInfo.pStages = shaderStagesInfos.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = subPassIndex;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    status = vkCreateGraphicsPipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");

    for (auto& shaderModule : shaderModules)
    {
        ShaderManager::destroyShaderModule(shaderModule);
    }
}

void SkyBox::createDescriptorSet()
{
    auto skyboxModel = getRenderResource()->m_skybox;
    uint32_t meshIndex = skyboxModel->getMeshIndices()[0];

    DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_descriptorSetLayout, &m_descriptorSet);

    VkDescriptorBufferInfo uniformBufferInfo = DescriptorManager::descriptorBufferInfo(m_ubo);
    VkDescriptorImageInfo skybox = DescriptorManager::descriptorImageInfo(getRenderResource()->m_skyboxCubeMap.sampler->getSampler(), getRenderResource()->m_skyboxCubeMap.image->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        DescriptorManager::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBufferInfo),
        DescriptorManager::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,&skybox)
    };
    vkUpdateDescriptorSets(getRendererPointer()->getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

}

void SkyBox::createUBO()
{
    //Skybox
    auto skybox = getRenderResource()->m_skybox;
    size_t uboSizeInfos =  sizeof(DescriptorTypes::UniformBufferObject::Skybox) ;

    uint32_t meshIndex = skybox->getMeshIndices()[0];
    {
        BufferManager::bufferCreateBuffer(
            getRendererPointer()->getVmaAllocator(),
            uboSizeInfos,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            &m_ubo,
            &m_uboAllocation
        );
    }
}

void SkyBox::updateUBO()
{
    VkExtent2D extent = getRendererPointer()->getSwapchainInfo().extent;

    auto skybox = getRenderResource()->m_skybox;
    for (uint32_t meshIndex : skybox->getMeshIndices())
    {
        DescriptorTypes::UniformBufferObject::Skybox newUBO;

        newUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(getRenderResource()->m_camera.getCameraPos()));
        newUBO.view = getRenderResource()->m_camera.getViewMatrix();
        newUBO.proj = MathUtils::getUpdatedProjMatrix(glm::radians(75.0f), extent.width / (float)extent.height, 0.01f, 40.0f);

        void* data;
        vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocation, &data);
        memcpy(data, &newUBO, sizeof(newUBO));
        vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocation);
    }
}

void SkyBox::draw(VkCommandBuffer & commandBuffer)
{
    VkExtent2D extent = getRendererPointer()->getSwapchainInfo().extent;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    // Set Dynamic States
    VkViewport viewport{ 0.0f, 0.0f, extent.width,extent.height, 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{ {0,0}, {extent.width,extent.height} };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    uint32_t meshIndex = getRenderResource()->m_skybox->getMeshIndices()[0];
    {
        RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];

        std::vector<VkDeviceSize> offsets = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, renderMeshInfo.ref_mesh->vertexBuffer, offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, *renderMeshInfo.ref_mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        const std::vector<VkDescriptorSet> sets = { m_descriptorSet };
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            sets.size(), sets.data(),
            0, {}
        );

        vkCmdDrawIndexed(commandBuffer, renderMeshInfo.ref_mesh->meshIndexCount, 1, 0, 0, 0);
    }
  
    
}

void SkyBox::destroy()
{
    vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_ubo, m_uboAllocation);

    vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayout, nullptr);
    vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayout, nullptr);
}
