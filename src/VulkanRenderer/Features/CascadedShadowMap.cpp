//#include "VulkanRenderer/Features/CascadedShadowMap.h"
//
//#include <memory>
//
//#include <vulkan/vulkan.h>
//
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <stdexcept>
//
//#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
//#include "VulkanRenderer/Settings/Config.h"
//#include "VulkanRenderer/Framebuffer/FramebufferManager.h"
//#include "VulkanRenderer/Math/MathUtils.h"
//#include "VulkanRenderer/Model/Attributes.h"
//#include "VulkanRenderer/RenderPass/AttachmentUtils.h"
//
//#include "VulkanRenderer/Command/CommandManager.h"
//#include "VulkanRenderer/Buffer/BufferManager.h"
//#include "VulkanRenderer/Descriptor/DescriptorTypes.h"
//#include "VulkanRenderer/Descriptor/DescriptorManager.h"
//#include "VulkanRenderer/Pipeline/PipelineManager.h"
//#include "VulkanRenderer/Shader/ShaderManager.h"
//
//#include "VulkanRenderer/Model/ModelManager.h"
//#include "VulkanRenderer/Renderer.h" 
//
//
//CascadedShadowMap::CascadedShadowMap(
//    const VkExtent2D& extent,
//    const uint32_t imagesCount,
//    const VkFormat& format,
//    const float& lambda,
//    const float& near_offset,
//    const uint32_t& split_count
//) : m_width(extent.width), m_height(extent.height),m_lambda(lambda),m_near_offset(near_offset),m_split_count(split_count)
//{
//    m_clearValuesCascadedShadowMap.resize(2);
//    m_clearValuesCascadedShadowMap[0].depthStencil.depth = 1.0f;
//    m_clearValuesCascadedShadowMap[1].depthStencil.depth = 1.0f;
//    m_clearValuesCascadedShadowMap[0].depthStencil.stencil = 0.0f;
//    m_clearValuesCascadedShadowMap[1].depthStencil.stencil = 0.0f;
//
//    m_texture = std::make_shared<NormalTexture>("CascadedShadowMap");
//    m_texture->getFormat() = format;
//    m_texture->getExtent() = VkExtent2D({ m_width,m_height });
//
//
//    BufferManager::bufferCreateOffscreenResources(
//        getRendererPointer()->getDevice(),
//        getRendererPointer()->getVmaAllocator(),
//        getRendererPointer()->getGraphicsQueue(),
//        m_texture->getExtent(),
//        m_texture->getFormat(),
//        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
//        1,
//        m_split_count,
//        0,
//        VK_SAMPLE_COUNT_1_BIT,
//        VK_IMAGE_VIEW_TYPE_2D,
//        &m_texture->getAllocation(),
//        m_texture
//    );
//
//
//    BufferManager::bufferTransitionImageLayout(
//        getRendererPointer()->getDevice(),
//        getRendererPointer()->getGraphicsQueue(),
//        getRendererPointer()->getCommandPool(),
//        m_texture->getImage(),
//        format,
//        VK_IMAGE_ASPECT_DEPTH_BIT,
//        1,
//        m_split_count,
//        VK_IMAGE_LAYOUT_UNDEFINED,
//        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
//    );
//
//
//
//    createRenderPass(format);
//    createFramebuffer(imagesCount);
//    createGraphicsPipeline(extent);
//
//    //Create UBOs && DescriptorSets PerMesh
//    createUBOs();
//    createDescriptorSets();
//
//
//    //Create CommandPool
//    CommandManager::cmdCreateCommandPool(
//        getRendererPointer()->getDevice(),
//        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
//        getRendererPointer()->getQueueFamilyIndices().graphicsFamily.value(),
//        &m_commandPool
//    );
//
//    //Create CommandBuffers
//    m_commandBuffers.resize(Config::MAX_FRAMES_IN_FLIGHT);
//    CommandManager::cmdCreateCommandBuffers(
//        getRendererPointer()->getDevice(),
//        m_commandPool,
//        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//        Config::MAX_FRAMES_IN_FLIGHT,
//        &m_commandBuffers[0]
//    );
//
//}
//
//
//void CascadedShadowMap::createGraphicsPipeline(const VkExtent2D& extent)
//{
//    //-------------------------------- Shadow Pipeline --------------------------------------
//    {
//        const std::vector<DescriptorInfo>& descriptorInfo = GRAPHICS_PIPELINE::SHADOWMAP::DESCRIPTORS_INFO;
//
//        std::vector<VkDescriptorSetLayoutBinding> bindings(descriptorInfo.size());
//        for (uint32_t i = 0; i < descriptorInfo.size(); i++)
//        {
//            bindings[i].binding = descriptorInfo[i].bindingNumber;
//            bindings[i].descriptorType = descriptorInfo[i].descriptorType;
//            bindings[i].descriptorCount = 1;
//            bindings[i].stageFlags = descriptorInfo[i].shaderStage;
//            bindings[i].pImmutableSamplers = nullptr;
//        }
//
//        VkDescriptorSetLayoutCreateInfo layoutInfo{};
//        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
//        layoutInfo.pBindings = bindings.data();
//
//        if (vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
//            throw std::runtime_error("Failed to create descriptor set layout!");
//
//
//        // -------------------Shader Modules--------------------
//        const std::vector<ShaderInfo>& shaderInfos = { {shaderType::VERTEX, "shadowMap"} };
//
//        std::vector<VkShaderModule> shaderModules(shaderInfos.size());
//        std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos(shaderInfos.size());
//        for (uint32_t i = 0; i < shaderInfos.size(); i++)
//        {
//            PipelineManager::createShaderModule(shaderInfos[i], shaderModules[i]);
//            PipelineManager::createShaderStageInfo(shaderModules[i], shaderInfos[i].type, shaderStagesInfos[i]);
//        }
//
//
//        // Dynamic states
//        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
//        VkPipelineDynamicStateCreateInfo dynamicState = PipelineManager::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());
//
//
//        // Viewport state info
//        VkPipelineViewportStateCreateInfo viewportState = PipelineManager::pipelineViewportStateCreateInfo(1, 1, 0);
//        // Vertex input(attributes)
//        std::vector<VkVertexInputAttributeDescription> attribDescription = Attributes::SHADOWMAP::getAttributeDescriptions();
//        VkVertexInputBindingDescription bindingDescription = Attributes::PBR::getBindingDescription();
//        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//        PipelineManager::createVertexShaderInputInfo(bindingDescription, attribDescription, vertexInputInfo);
//        // Input assembly
//        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = PipelineManager::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
//        // Rasterizer
//        VkPipelineRasterizationStateCreateInfo rasterizationState = PipelineManager::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0, VK_TRUE, 4.0f, 1.5f);
//        // Multisampling 
//        VkPipelineMultisampleStateCreateInfo multisampleState = PipelineManager::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
//        // Color blending(attachment)
//        VkPipelineColorBlendAttachmentState blendAttachmentState = PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
//        // Color blending(global)
//        VkPipelineColorBlendStateCreateInfo colorBlendState = PipelineManager::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
//
//        // Pipeline layout
//        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//        pipelineLayoutInfo.setLayoutCount = 1;
//        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
//        pipelineLayoutInfo.pushConstantRangeCount = 0;
//        pipelineLayoutInfo.pPushConstantRanges = nullptr;
//        auto status = vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
//        if (status != VK_SUCCESS)
//            throw std::runtime_error("Failed to create pipeline layout!");
//
//        // Depth and stencil
//        VkPipelineDepthStencilStateCreateInfo depthStencilState = PipelineManager::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
//
//        VkGraphicsPipelineCreateInfo pipelineInfo{};
//        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//        pipelineInfo.stageCount = shaderInfos.size();
//        pipelineInfo.pStages = shaderStagesInfos.data();
//        pipelineInfo.pVertexInputState = &vertexInputInfo;
//        pipelineInfo.pInputAssemblyState = &inputAssemblyState;
//        pipelineInfo.pViewportState = &viewportState;
//        pipelineInfo.pRasterizationState = &rasterizationState;
//        pipelineInfo.pMultisampleState = &multisampleState;
//        pipelineInfo.pDepthStencilState = &depthStencilState;
//        pipelineInfo.pColorBlendState = &colorBlendState;
//        pipelineInfo.pDynamicState = &dynamicState;
//        pipelineInfo.pTessellationState = nullptr;
//        pipelineInfo.pNext = nullptr;
//        pipelineInfo.layout = m_pipelineLayout;
//        pipelineInfo.renderPass = m_renderPass.get();
//        pipelineInfo.subpass = 0;
//        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
//        pipelineInfo.basePipelineIndex = -1;
//
//        status = vkCreateGraphicsPipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
//        if (status != VK_SUCCESS)
//            throw std::runtime_error("Failed to create graphics pipeline!");
//
//        for (auto& shaderModule : shaderModules)
//        {
//            ShaderManager::destroyShaderModule(shaderModule);
//        }
//    }
//}
//
//
//CascadedShadowMap::~CascadedShadowMap() {}
//
//
//
//void CascadedShadowMap::updateUBO()
//{
//    for (auto ptr : getRenderResource()->m_normalModels)
//    {
//
//        for (uint32_t meshIndex : ptr->getMeshIndices())
//        {
//            m_basicInfo.model = ptr->getModelMatrix();
//
//            //glm::mat4 proj = MathUtils::getUpdatedProjMatrix(glm::radians(Config::FOV), 1.0, Config::Z_NEAR_SHADOW, Config::Z_FAR_SHADOW);
//            glm::mat4 proj = glm::ortho(-8.0f, 8.0f, -8.0f, 8.0f, 0.5f, 50.0f);
//
//            proj[1][1] *= -1;
//
//            LightInfo& info = getRenderResource()->m_lightsInfo[getRenderResource()->m_directionalLightIndex];
//
//            glm::fvec3 lightDir = glm::normalize(info.m_targetPos - info.pos);
//
//            glm::mat4 view = glm::lookAt(info.pos, info.pos + lightDir, glm::fvec3(0.0f, 1.0f, 0.0f));
//
//            m_basicInfo.lightSpace = proj * view;
//
//            void* data;
//            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex], &data);
//            memcpy(data, &m_basicInfo, sizeof(m_basicInfo));
//            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex]);
//        }
//
//    }
//}
//
//void CascadedShadowMap::draw(uint32_t imageIndex, uint32_t currentFrame)
//{
//    VkCommandBuffer& commandBuffer = getRendererPointer()->getGraphicsCommandBuffer(currentFrame);
//
//    //--------------------------------RenderPass-----------------------------
//    m_renderPass.begin(m_framebuffers[imageIndex], VkExtent2D({ m_width,m_height }), m_clearValuesCascadedShadowMap, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
//
//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
//
//    // Set Dynamic States
//    VkViewport viewport{ 0.0f, 0.0f,m_width,m_height, 0.0f, 1.0f };
//    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
//
//    VkRect2D scissor{ {0,0}, {m_width,m_height} };
//    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
//    {
//        for (auto ptr : getRenderResource()->m_normalModels)
//        {
//            for (uint32_t meshIndex : ptr->getMeshIndices())
//            {
//                RenderMeshInfo& meshInfo = getRenderResource()->m_meshInfoMap[meshIndex];
//
//                std::vector<VkDeviceSize> offsets = { 0 };
//                vkCmdBindVertexBuffers(commandBuffer, 0, 1, meshInfo.ref_mesh->vertexBuffer, offsets.data());
//                vkCmdBindIndexBuffer(commandBuffer, *meshInfo.ref_mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//
//                const std::vector<VkDescriptorSet> sets = { m_descriptorSetsMap[meshIndex] };
//                vkCmdBindDescriptorSets(
//                    commandBuffer,
//                    VK_PIPELINE_BIND_POINT_GRAPHICS,
//                    m_pipelineLayout,
//                    0,
//                    sets.size(), sets.data(),
//                    0, {}
//                );
//
//                vkCmdDrawIndexed(commandBuffer, meshInfo.ref_mesh->meshIndexCount, 1, 0, 0, 0);
//            }
//        }
//    }
//    m_renderPass.end(commandBuffer);
//}
//
//
//
//const VkCommandBuffer& CascadedShadowMap::getCommandBuffer(const uint32_t index) const
//{
//    return m_commandBuffers[index];
//}
//
//const std::shared_ptr<TextureBase> CascadedShadowMap::get() const
//{
//    return m_texture;
//}
//
//
//const VkImageView& CascadedShadowMap::getCascadedShadowMapView() const
//{
//    return m_texture->getImageView();
//}
//
//
//const VkSampler& CascadedShadowMap::getSampler() const
//{
//    return m_texture->getSampler();
//}
//
//const VkFramebuffer& CascadedShadowMap::getFramebuffer(const uint32_t imageIndex) const
//{
//    return m_framebuffers[imageIndex];
//}
//
//
//const VkCommandPool& CascadedShadowMap::getCommandPool() const
//{
//    return m_commandPool;
//}
//
//
//const glm::mat4& CascadedShadowMap::getLightSpace() const
//{
//    return m_basicInfo.lightSpace;
//}
//
//
//const RenderPass& CascadedShadowMap::getRenderPass() const
//{
//    return m_renderPass;
//}
//
//
//
//void CascadedShadowMap::createFramebuffer(const uint32_t& imagesCount)
//{
//
//    m_framebuffers.resize(imagesCount);
//    std::vector<VkImageView> attachments = { m_texture->getImageView() };
//
//    for (uint32_t i = 0; i < imagesCount; i++)
//    {
//        FramebufferManager::createFramebuffer(getRendererPointer()->getDevice(), m_renderPass.get(), attachments, m_width, m_height, 1, &m_framebuffers[i]);
//    }
//}
//
//void CascadedShadowMap::createUBOs()
//{
//    for (auto ptr : getRenderResource()->m_normalModels)
//    {
//        for (uint32_t meshIndex : ptr->getMeshIndices())
//        {
//            BufferManager::bufferCreateBuffer(
//                getRendererPointer()->getVmaAllocator(),
//                sizeof(DescriptorTypes::UniformBufferObject::ShadowMap),
//                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
//                VMA_MEMORY_USAGE_CPU_TO_GPU,
//                &m_ubosMap[meshIndex],
//                &m_uboAllocationsMap[meshIndex]
//            );
//        }
//    }
//}
//
//void CascadedShadowMap::createDescriptorSets()
//{
//    //------------------------------- DescriptorSets  ----------------------------------
//    for (auto ptr : getRenderResource()->m_normalModels)
//    {
//        for (uint32_t meshIndex : ptr->getMeshIndices())
//        {
//            m_descriptorSetsMap[meshIndex];
//            {
//                DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_descriptorSetLayout, &m_descriptorSetsMap[meshIndex]);
//
//                VkDescriptorBufferInfo uniformBufferInfo = DescriptorManager::descriptorBufferInfo(m_ubosMap[meshIndex]);
//
//                std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
//                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBufferInfo),
//                };
//                vkUpdateDescriptorSets(getRendererPointer()->getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
//            }
//        }
//    }
//
//}
//
//
//
//
//void CascadedShadowMap::destroy()
//{
//    vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayout, nullptr);
//    vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipeline, nullptr);
//    vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayout, nullptr);
//
//
//    m_texture->destroy();
//
//
//    for (auto& uboInfo : m_ubosMap)
//    {
//        vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), uboInfo.second, m_uboAllocationsMap[uboInfo.first]);
//    }
//
//
//    vkDestroyCommandPool(getRendererPointer()->getDevice(), m_commandPool, nullptr);
//
//    m_renderPass.destroy();
//
//    for (auto& framebuffer : m_framebuffers)
//        vkDestroyFramebuffer(getRendererPointer()->getDevice(), framebuffer, nullptr);
//}
//
//
//void CascadedShadowMap::createRenderPass(const VkFormat& depthBufferFormat)
//{
//    // - Attachments
//    VkAttachmentDescription shadowMapAttachment{};
//    AttachmentUtils::createAttachmentDescriptionWithStencil(
//        depthBufferFormat,
//        VK_SAMPLE_COUNT_1_BIT,
//        VK_ATTACHMENT_LOAD_OP_CLEAR,
//        VK_ATTACHMENT_STORE_OP_STORE,
//        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//        VK_ATTACHMENT_STORE_OP_DONT_CARE,
//        VK_IMAGE_LAYOUT_UNDEFINED,
//        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//        shadowMapAttachment
//    );
//
//
//    // Attachment references
//    VkAttachmentReference shadowMapAttachmentRef{};
//    AttachmentUtils::createAttachmentReference(
//        0,
//        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
//        shadowMapAttachmentRef
//    );
//
//    // Subpasses
//    VkSubpassDescription subpass = {};
//    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//    subpass.flags = 0;
//    subpass.pDepthStencilAttachment = &shadowMapAttachmentRef;
//
//    m_renderPass = RenderPass(
//        { shadowMapAttachment },
//        { subpass },
//        {}
//        //dependencies
//    );
//
//}
