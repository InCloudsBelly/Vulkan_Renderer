#include "VulkanRenderer/Scene/Spherical_Harmonic_Lighting.h"

#include <iostream>

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Pipeline/PipelineManager.h"
#include "VulkanRenderer/Shader/ShaderManager.h"
#include "VulkanRenderer/Math/MathUtils.h"

SHLightingPass::SHLightingPass()
{
    m_extent = getRendererPointer()->getSwapchainInfo().extent;
    // Clear Color
    m_clearValues.resize(2);
    m_clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[1].color = { 1.0f, 0.0f };

    createRenderPass();
    createPipelines();

    //Secondary Features
    createSecondaryFeatures();

    createUBOs();
    createDescriptorSets();


    createSwapchainFramebuffers();
}

SHLightingPass::~SHLightingPass() {}


void SHLightingPass::createRenderPass()
{
    VkFormat format = getRendererPointer()->getSwapchainInfo().image_format;
    VkFormat depthBufferFormat = getRendererPointer()->getDepthImageInfo().depth_image_format;
    VkSampleCountFlagBits msaaSamplesCount = getRendererPointer()->getMSAAInfo().msaa_sampleCount;

    // - Attachments

    // Color Attachment
    VkAttachmentDescription colorAttachment{};
    AttachmentUtils::createAttachmentDescription(
        format,
        msaaSamplesCount,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorAttachment
    );


    // Depth Attachment
    VkAttachmentDescription depthAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        depthBufferFormat,
        msaaSamplesCount,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        // We don't care about storing the depth data, because it will not be used after drawing has finished.
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        // Just like the color buffer, we don't care about the previous depth contents.
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        depthAttachment
    );


    //Color Resolve Attachment (needed by MSAA)
    VkAttachmentDescription colorResolveAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        // Here is not 'VK_IMAGE_LAYOUT_PRESENT_SRC_KHR'
        // because the GUI will be the last and the one to present.
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorResolveAttachment
    );


    // - Attachment References
    VkAttachmentReference colorAttachmentRef{};
    AttachmentUtils::createAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, colorAttachmentRef);

    VkAttachmentReference depthAttachmentRef{};
    AttachmentUtils::createAttachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depthAttachmentRef);

    VkAttachmentReference colorResolveAttachmentRef{};
    AttachmentUtils::createAttachmentReference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, colorResolveAttachmentRef);



    // - Subpasses
    VkSubpassDescription subPassDescript{};
    SubPassUtils::createSubPassDescription(
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        &colorAttachmentRef,
        &depthAttachmentRef,
        &colorResolveAttachmentRef,
        subPassDescript
    );



    // - Subpass dependices
    VkSubpassDependency dependency{};
    SubPassUtils::createSubPassDependency(
        // -Source parameters.
        // VK_SUBPASS_EXTERNAL means anything outside of a given render pass scope. 
        // When used for srcSubpass it specifies anything that happened before the render pass. 
        VK_SUBPASS_EXTERNAL,
        (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT),
        0,
        0,
        (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT),
        (VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        (VkDependencyFlagBits)0,
        dependency
    );



    m_renderPass = RenderPass(
        { colorAttachment, depthAttachment, colorResolveAttachment },
        { subPassDescript },
        { dependency }
    );
}

void SHLightingPass::createSwapchainFramebuffers()
{
    m_swapchain_framebuffers.resize(getRendererPointer()->getSwapchainInfo().imageViews.size());

    // create frame buffer for every imageview
    for (size_t i = 0; i < m_swapchain_framebuffers.size(); i++)
    {
        m_swapchain_framebuffers[i] = new VkFramebuffer;

        std::vector<VkImageView> attachments = {
            *getRendererPointer()->getMSAAInfo().msaa_image_view,
            *getRendererPointer()->getDepthImageInfo().depth_image_view,
            *getRendererPointer()->getSwapchainInfo().imageViews[i]
        };

        FramebufferManager::createFramebuffer(
            getRendererPointer()->getDevice(),
            m_renderPass.get(),
            attachments,
            m_extent.width,
            m_extent.height,
            1,
            m_swapchain_framebuffers[i]
        );
    }
}

void SHLightingPass::createSecondaryFeatures()
{
   

    uint32_t subPassIndex = 0;
    m_skyBox = std::make_shared<SkyBox>(m_renderPass.get(), getRendererPointer()->getMSAAInfo().msaa_sampleCount, subPassIndex);

    //GUI
    m_GUI = std::make_unique<GUI>();

    // IBL
    loadSHBRDFlut();
    
}


void SHLightingPass::createPipelines()
{
    m_pipelines.resize(PipelineIndex::PIPELINE_NUM);
    m_descriptorSetLayouts.resize(PipelineIndex::PIPELINE_NUM);
    m_pipelineLayouts.resize(PipelineIndex::PIPELINE_NUM);

    VkSampleCountFlagBits msaaSamplesCount = getRendererPointer()->getMSAAInfo().msaa_sampleCount;

    //-------------------------------- PBR Pipeline --------------------------------------
    {
        const std::vector<DescriptorInfo>& descriptorInfo = GRAPHICS_PIPELINE::SH_LIGHTING::DESCRIPTORS_INFO;

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

        if (vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayouts[PipelineIndex::main_pipeline]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout!");


        // -------------------Shader Modules--------------------
        const std::vector<ShaderInfo>& shaderInfos = { {shaderType::VERTEX, "shLighting"}, {shaderType::FRAGMENT, "shLighting"} };

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

        // Viewport
        VkViewport viewport{};
        PipelineManager::createViewport(viewport, m_extent);
        // Scissor
        VkRect2D scissor{};
        PipelineManager::createScissor(scissor, m_extent);
        // Viewport state info
        VkPipelineViewportStateCreateInfo viewportState = PipelineManager::pipelineViewportStateCreateInfo(1, 1, 0);
        // Vertex input(attributes)
        std::vector<VkVertexInputAttributeDescription> attribDescription = Attributes::PBR::getAttributeDescriptions();
        VkVertexInputBindingDescription bindingDescription = Attributes::PBR::getBindingDescription();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        PipelineManager::createVertexShaderInputInfo(bindingDescription, attribDescription, vertexInputInfo);
        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = PipelineManager::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizationState = PipelineManager::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0, VK_FALSE);
        // Multisampling 
        VkPipelineMultisampleStateCreateInfo multisampleState = PipelineManager::pipelineMultisampleStateCreateInfo(msaaSamplesCount, 0);
        // Color blending(attachment)
        VkPipelineColorBlendAttachmentState blendAttachmentState = PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        // Color blending(global)
        VkPipelineColorBlendStateCreateInfo colorBlendState = PipelineManager::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayouts[PipelineIndex::main_pipeline];
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        auto status = vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayouts[PipelineIndex::main_pipeline]);
        if (status != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");

        // Depth and stencil
        VkPipelineDepthStencilStateCreateInfo depthStencilState = PipelineManager::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

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
        pipelineInfo.layout = m_pipelineLayouts[PipelineIndex::main_pipeline];
        pipelineInfo.renderPass = m_renderPass.get();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        status = vkCreateGraphicsPipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipelines[PipelineIndex::main_pipeline]);
        if (status != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline!");

        for (auto& shaderModule : shaderModules)
        {
            ShaderManager::destroyShaderModule(shaderModule);
        }
    }
}


void SHLightingPass::updateUBO(
    const VkExtent2D& extent,
    const uint32_t& currentFrame
) {
    m_skyBox->updateUBO();

    UBOinfo uboInfo = {
        getRenderResource()->m_camera.getCameraPos(),
        getRenderResource()->m_camera.getViewMatrix(),
        getRenderResource()->m_camera.getProjectionMatrix(),
        glm::mat4(1.0f),
        getRenderResource()->m_lightsInfo.size(),
        extent
    };

    // SHLightingPass

    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            // update normal UBO 
            {
                DescriptorTypes::UniformBufferObject::NormalPBR  uboData1;
                uboData1.model = ptr->getModelMatrix();
                uboData1.view = uboInfo.view;
                uboData1.proj = uboInfo.proj;
                uboData1.lightSpace = uboInfo.lightSpace;

                uboData1.cameraPos = glm::vec4(uboInfo.cameraPos, 1.0f);
                uboData1.lightsCount = uboInfo.lightsCount;

                void* data;
                vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_meshesUBOAllocationMap[meshIndex][0], &data);
                memcpy(data, &uboData1, sizeof(uboData1));
                vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_meshesUBOAllocationMap[meshIndex][0]);
            }

            // SH UBOs
            {
                glm::vec4 coefficentData[Config::SH_COEF_NUM];

                for (uint32_t i = 0; i < Config::SH_COEF_NUM; i++)
                {
                    coefficentData[i] = glm::vec4(getRenderResource()->m_coefficient[i], 1.0f);
                }

                void* data;
                vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_meshesUBOAllocationMap[meshIndex][1], &data);
                memcpy(data, &coefficentData, sizeof(coefficentData[0]) * Config::SH_COEF_NUM);
                vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_meshesUBOAllocationMap[meshIndex][1]);
            }
        }
    }
}

void SHLightingPass::draw(uint32_t imageIndex, uint32_t currentFrame)
{
    VkCommandBuffer& commandBuffer = getRendererPointer()->getGraphicsCommandBuffer(currentFrame);
    // Resets the command buffer to be able to be recorded.
    vkResetCommandBuffer(commandBuffer, 0);

    // Specifies some details about the usage of this specific command buffer.
    CommandManager::cmdBeginCommandBuffer(commandBuffer, (VkCommandBufferUsageFlagBits)0);



    //--------------------------------RenderPass-----------------------------
    VkExtent2D extent = getRendererPointer()->getSwapchainInfo().extent;
    m_renderPass.begin(*m_swapchain_framebuffers[imageIndex], extent, m_clearValues, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

    drawPipeline(commandBuffer, m_pipelines[PipelineIndex::main_pipeline], m_pipelineLayouts[PipelineIndex::main_pipeline], getRenderResource()->m_normalModels);

    m_skyBox->draw(commandBuffer);

    m_renderPass.end(commandBuffer);

    //GUI
    m_GUI->draw(currentFrame, imageIndex);

    vkEndCommandBuffer(commandBuffer);
}

void SHLightingPass::createUBOs()
{
    //Normal models
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        std::vector<size_t> uboSizeInfos = {
               sizeof(DescriptorTypes::UniformBufferObject::NormalPBR),
               sizeof(glm::vec4) * Config::SH_COEF_NUM
        };
        createUniformBuffer(ptr, uboSizeInfos);
    }
}

void SHLightingPass::createDescriptorSets()
{
    //-------------------------------  PBR DescriptorSet  ----------------------------------
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            m_meshesDescriptorSetMap[meshIndex].resize(1);

            {
                DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_descriptorSetLayouts[PipelineIndex::main_pipeline], &m_meshesDescriptorSetMap[meshIndex][0]);

                RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];

                VkDescriptorBufferInfo uniformBufferInfo = DescriptorManager::descriptorBufferInfo(m_meshesUBOMap[meshIndex][0]);
                VkDescriptorBufferInfo uniformBufferSHcoefficientInfo = DescriptorManager::descriptorBufferInfo(m_meshesUBOMap[meshIndex][1]);


                // SH Lighting with PBR info semms to be 
                VkDescriptorImageInfo baseColor = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->colorTexture->getSampler(), renderMeshInfo.ref_material->colorTexture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo metallicRoughness = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->metallic_RoughnessTexture->getSampler(), renderMeshInfo.ref_material->metallic_RoughnessTexture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo emissiveColor = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->emissiveTexture->getSampler(), renderMeshInfo.ref_material->emissiveTexture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo AO = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->AOTexture->getSampler(), renderMeshInfo.ref_material->AOTexture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo normal = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->normalTexture->getSampler(), renderMeshInfo.ref_material->normalTexture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                VkDescriptorImageInfo SHBRDFlut = DescriptorManager::descriptorImageInfo(getRenderResource()->m_SHBRDFlut->getSampler(), getRenderResource()->m_SHBRDFlut->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
               


                std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                    DescriptorManager::writeDescriptorSet(m_meshesDescriptorSetMap[meshIndex][0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBufferInfo),
                    DescriptorManager::writeDescriptorSet(m_meshesDescriptorSetMap[meshIndex][0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &uniformBufferSHcoefficientInfo),

                    DescriptorManager::writeDescriptorSet(m_meshesDescriptorSetMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &baseColor),
                    DescriptorManager::writeDescriptorSet(m_meshesDescriptorSetMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &metallicRoughness),
                    DescriptorManager::writeDescriptorSet(m_meshesDescriptorSetMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &emissiveColor),
                    DescriptorManager::writeDescriptorSet(m_meshesDescriptorSetMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &AO),
                    DescriptorManager::writeDescriptorSet(m_meshesDescriptorSetMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, &normal),

                    DescriptorManager::writeDescriptorSet(m_meshesDescriptorSetMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7,&SHBRDFlut)
                };

                vkUpdateDescriptorSets(getRendererPointer()->getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
            }
        }
    }
}


void SHLightingPass::destroy()
{
    for (auto& framebuffer : m_swapchain_framebuffers)
        vkDestroyFramebuffer(getRendererPointer()->getDevice(), *framebuffer, nullptr);

    for (auto& uboInfo : m_meshesUBOMap)
    {
        for (uint32_t i = 0; i < uboInfo.second.size(); ++i)
        {
            vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), uboInfo.second[i], m_meshesUBOAllocationMap[uboInfo.first][i]);
        }
    }

    // ImGui
    m_GUI->destroy();
    m_skyBox->destroy();

    vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayouts[PipelineIndex::main_pipeline], nullptr);
    vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipelines[PipelineIndex::main_pipeline], nullptr);
    vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayouts[PipelineIndex::main_pipeline], nullptr);

    m_renderPass.destroy();
}



void SHLightingPass::loadSHBRDFlut()
{
    std::string TextureName = "SH_BRDF_LUT.png";
    TextureToLoadInfo info = { TextureName,"/defaultTextures",VK_FORMAT_R8G8B8A8_SRGB,4 };

    getRenderResource()->m_SHBRDFlut = std::make_shared<NormalTexture>(TextureName, std::string(MODEL_DIR) + info.folderName, info.format);
}
