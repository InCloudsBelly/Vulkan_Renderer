#include "VulkanRenderer/Scene/ForwardPBR.h"

#include <iostream>

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Pipeline/PipelineManager.h"
#include "VulkanRenderer/Shader/ShaderManager.h"
#include "VulkanRenderer/Math/MathUtils.h"

ForwardPBRPass::ForwardPBRPass() 
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

    initComputations();

    createSwapchainFramebuffers();
}

void ForwardPBRPass::initComputations() 
{
    m_BRDFcomp = Computation(
        "BRDF",
        sizeof(float),
        2 * sizeof(float) * Config::BRDF_HEIGHT * Config::BRDF_WIDTH,
        getRendererPointer()->getQueueFamilyIndices(),
        getRendererPointer()->getDescriptorPool(),
        COMPUTE_PIPELINE::BRDF::BUFFERS_INFO
    );
}

ForwardPBRPass::~ForwardPBRPass() {}


void ForwardPBRPass::createRenderPass()
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
        (VkDependencyFlagBits) 0,
        dependency
    );



    m_renderPass = RenderPass(
        { colorAttachment, depthAttachment, colorResolveAttachment },
        { subPassDescript },
        { dependency }
    );
}

void ForwardPBRPass::createSwapchainFramebuffers()
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

void ForwardPBRPass::createSecondaryFeatures()
{
    //ShadowMap
    const VkExtent2D swapChainExtent = getRendererPointer()->getSwapchainInfo().extent;
    const VkExtent2D shadowExtent = { 2 * swapChainExtent.height, 2 * swapChainExtent.width };

    m_shadowMap = std::make_shared<ShadowMap>(
        shadowExtent,
        getRendererPointer()->getSwapchainInfo().imageViews.size(),
        getRendererPointer()->getDepthImageInfo().depth_image_format,
        Config::MAX_FRAMES_IN_FLIGHT
        );

    uint32_t subPassIndex = 0;
    m_skyBox = std::make_shared<SkyBox>(m_renderPass.get(), getRendererPointer()->getMSAAInfo().msaa_sampleCount, subPassIndex);
    m_lightSphere = std::make_shared<LightSphere>(m_renderPass.get(), getRendererPointer()->getMSAAInfo().msaa_sampleCount, subPassIndex);
    
    //GUI
    m_GUI = std::make_unique<GUI>();

    // IBL
    loadBRDFlut();
    m_prefilteredIrradiance = std::make_shared<PrefilteredIrradiance>(Config::PREF_IRRADIANCE_DIM);
    m_prefilteredEnvMap = std::make_shared<PrefilteredEnvMap>(Config::PREF_ENV_MAP_DIM);

    getRenderResource()->updateIBLResource(m_BRDFlut, m_prefilteredIrradiance->get(), m_prefilteredEnvMap->get());

}

void ForwardPBRPass::createUniformBuffer(const std::shared_ptr<Model> modelPtr, std::vector<size_t>& uboSizeInfos)
{
    for (uint32_t meshIndex : modelPtr->getMeshIndices())
    {
        // create UBO PerMesh
        m_ubosMap[meshIndex].resize(uboSizeInfos.size());
        m_uboAllocationsMap[meshIndex].resize(uboSizeInfos.size());

        for (uint32_t i = 0; i < uboSizeInfos.size(); ++i)
        {
            BufferManager::bufferCreateBuffer(
                getRendererPointer()->getVmaAllocator(),
                uboSizeInfos[i],
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                &m_ubosMap[meshIndex][i],
                &m_uboAllocationsMap[meshIndex][i]
            );
        }
    }
}



void ForwardPBRPass::createPipelines()
{
    VkSampleCountFlagBits msaaSamplesCount = getRendererPointer()->getMSAAInfo().msaa_sampleCount;

    //-------------------------------- PBR Pipeline --------------------------------------
    {
        const std::vector<DescriptorInfo>& descriptorInfo = GRAPHICS_PIPELINE::PBR::DESCRIPTORS_INFO;

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

        if (vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayoutPBR) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout!");


        // -------------------Shader Modules--------------------
        const std::vector<ShaderInfo>& shaderInfos = { {shaderType::VERTEX, "scene"}, {shaderType::FRAGMENT, "scene"} };

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
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayoutPBR;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        auto status = vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayoutPBR);
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
        pipelineInfo.layout = m_pipelineLayoutPBR;
        pipelineInfo.renderPass = m_renderPass.get();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        status = vkCreateGraphicsPipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipelinePBR);
        if (status != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline!");

        for (auto& shaderModule : shaderModules)
        {
            ShaderManager::destroyShaderModule(shaderModule);
        }
    }
}


void ForwardPBRPass::updateUBO(
    const VkExtent2D& extent,
    const uint32_t& currentFrame
) {
    m_shadowMap->updateUBO();
    m_lightSphere->updateUBO();
    m_skyBox->updateUBO();

    const glm::mat4 lightSpace1 = m_shadowMap->getLightSpace();

    UBOinfo uboInfo = {
        getRenderResource()->m_camera.getCameraPos(),
        getRenderResource()->m_camera.getViewMatrix(),
        getRenderResource()->m_camera.getProjectionMatrix(),
        lightSpace1,
        getRenderResource()->m_lightsInfo.size(),
        extent
    };

    // ForwardPBRPass

    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            // update normal UBO 
            DescriptorTypes::UniformBufferObject::NormalPBR  uboData1;
            uboData1.model = ptr->getModelMatrix();
            uboData1.view = uboInfo.view;
            uboData1.proj = uboInfo.proj;
            uboData1.lightSpace = uboInfo.lightSpace;

            uboData1.cameraPos =glm::vec4(uboInfo.cameraPos, 1.0f);
            uboData1.lightsCount = uboInfo.lightsCount;

            void* data1;
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0], &data1);
            memcpy(data1, &uboData1, sizeof(uboData1));
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0]);


            // update lights UBO
            DescriptorTypes::UniformBufferObject::LightInfo uboData2[Config::LIGHTS_COUNT];

            for (uint32_t i = 0; i < getRenderResource()->m_lightsInfo.size(); i++)
            {
                LightInfo info = getRenderResource()->m_lightsInfo[i];
                uboData2[i].pos = glm::vec4(info.pos, 1.0f);
                uboData2[i].color = glm::vec4(info.m_color, 1.0f);
                uboData2[i].dir = glm::vec4(info.m_targetPos - info.pos, 1.0f);
                uboData2[i].intensity = info.m_intensity;
                uboData2[i].type = (int)info.m_lightType;
            }

            void* data2;
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][1], &data2);
            memcpy(data2, &uboData2, sizeof(uboData2[0]) * 10);
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][1]);
        }
    }
}

void ForwardPBRPass::draw(uint32_t imageIndex, uint32_t currentFrame)
{
    VkCommandBuffer& commandBuffer = getRendererPointer()->getGraphicsCommandBuffer(currentFrame);
    // Resets the command buffer to be able to be recorded.
    vkResetCommandBuffer(commandBuffer, 0);

    // Specifies some details about the usage of this specific command buffer.
    CommandManager::cmdBeginCommandBuffer(commandBuffer, (VkCommandBufferUsageFlagBits)0);

    //ShadowMap
    m_shadowMap->draw(imageIndex, currentFrame);

    //--------------------------------RenderPass-----------------------------
    VkExtent2D extent = getRendererPointer()->getSwapchainInfo().extent;
    m_renderPass.begin(*m_swapchain_framebuffers[imageIndex],extent , m_clearValues, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    
    drawPipeline(commandBuffer, m_pipelinePBR, m_pipelineLayoutPBR, getRenderResource()->m_normalModels);


    m_lightSphere->draw(commandBuffer);
    m_skyBox->draw(commandBuffer);
    
    m_renderPass.end(commandBuffer);

    //GUI
    m_GUI->draw(currentFrame, imageIndex);

    vkEndCommandBuffer(commandBuffer);
}

void ForwardPBRPass::createUBOs()
{
    //Normal models
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        std::vector<size_t> uboSizeInfos = {
               sizeof(DescriptorTypes::UniformBufferObject::NormalPBR),
               sizeof(DescriptorTypes::UniformBufferObject::LightInfo) * 10
        };
        createUniformBuffer(ptr, uboSizeInfos);
    }
}

void ForwardPBRPass::createDescriptorSets()
{
    //-------------------------------  PBR DescriptorSet  ----------------------------------
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            m_descriptorSetsMap[meshIndex].resize(1);

            {
                DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_descriptorSetLayoutPBR, &m_descriptorSetsMap[meshIndex][0]);

                RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];

                VkDescriptorBufferInfo uniformBufferInfo = DescriptorManager::descriptorBufferInfo(m_ubosMap[meshIndex][0]);
                VkDescriptorBufferInfo uniformBufferLightsInfo = DescriptorManager::descriptorBufferInfo(m_ubosMap[meshIndex][1]);

                VkDescriptorImageInfo baseColor         = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->colorTexture->getSampler(),               renderMeshInfo.ref_material->colorTexture->getImageView(),              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo metallicRoughness = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->metallic_RoughnessTexture->getSampler(),  renderMeshInfo.ref_material->metallic_RoughnessTexture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo emissiveColor     = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->emissiveTexture->getSampler(),            renderMeshInfo.ref_material->emissiveTexture->getImageView(),           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo AO                = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->AOTexture->getSampler(),                  renderMeshInfo.ref_material->AOTexture->getImageView(),                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo normal            = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->normalTexture->getSampler(),              renderMeshInfo.ref_material->normalTexture->getImageView(),             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                
                VkDescriptorImageInfo irradianceMap     = DescriptorManager::descriptorImageInfo(getRenderResource()->m_IBLResource.irradiance->getSampler(),           getRenderResource()->m_IBLResource.irradiance->getImageView(),          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo BRDFlut           = DescriptorManager::descriptorImageInfo(getRenderResource()->m_IBLResource.brdfLUT->getSampler(),              getRenderResource()->m_IBLResource.brdfLUT->getImageView(),             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo prefilteredEnvMap = DescriptorManager::descriptorImageInfo(getRenderResource()->m_IBLResource.prefiltered_Env->getSampler(),      getRenderResource()->m_IBLResource.prefiltered_Env->getImageView(),     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                VkDescriptorImageInfo shadowMap         = DescriptorManager::descriptorImageInfo(m_shadowMap->get()->getSampler(),                                      m_shadowMap->get()->getImageView(),                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


                std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBufferInfo),
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &uniformBufferLightsInfo),

                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &baseColor),
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &metallicRoughness),
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &emissiveColor),
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &AO),
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, &normal),

                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7, &irradianceMap),
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8, &BRDFlut),
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 9, &prefilteredEnvMap),

                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10,&shadowMap)
                };

                vkUpdateDescriptorSets(getRendererPointer()->getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
            }
        }
    }
}


const RenderPass& ForwardPBRPass::getRenderPass() const
{
    return m_renderPass;
}


void ForwardPBRPass::destroy()
{
    for (auto& framebuffer : m_swapchain_framebuffers)
        vkDestroyFramebuffer(getRendererPointer()->getDevice(), *framebuffer, nullptr);

    for (auto& uboInfo : m_ubosMap)
    {
        for (uint32_t i = 0; i < uboInfo.second.size(); ++i)
        {
            vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), uboInfo.second[i], m_uboAllocationsMap[uboInfo.first][i]);
        }
    }

    // ImGui
    m_GUI->destroy();
    m_shadowMap->destroy();
    m_skyBox->destroy();
    m_lightSphere->destroy();

    vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayoutPBR, nullptr);
    vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipelinePBR, nullptr);
    vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayoutPBR, nullptr);

    m_renderPass.destroy();

    // IBL
    m_BRDFcomp.destroy();
    m_BRDFlut->destroy();
    m_prefilteredIrradiance->destroy();
    m_prefilteredEnvMap->destroy();
}

// In the future, it'll return a vector of computations.
const Computation& ForwardPBRPass::getComputation() const
{
    return m_BRDFcomp;
}


void ForwardPBRPass::loadBRDFlut() 
{

    std::string TextureName = "BRDF_LUT.png";
    TextureToLoadInfo info = { TextureName,"/defaultTextures",VK_FORMAT_R8G8B8A8_SRGB,4 };

    m_BRDFlut = std::make_shared<NormalTexture>(TextureName,std::string(MODEL_DIR) + info.folderName,info.format);

}

void ForwardPBRPass::drawPipeline(const VkCommandBuffer& commandBuffer, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, std::vector<std::shared_ptr<Model>> models)
{

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Set Dynamic States
    VkViewport viewport{ 0.0f, 0.0f, m_extent.width,m_extent.height, 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{ {0,0}, {m_extent.width,m_extent.height} };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    for (auto& ptr : models)
    {
        if (ptr->isHidden() == false)
        {
            for (uint32_t meshIndex : ptr->getMeshIndices())
            {
                RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];

                std::vector<VkDeviceSize> offsets = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, renderMeshInfo.ref_mesh->vertexBuffer, offsets.data());
                vkCmdBindIndexBuffer(commandBuffer, *renderMeshInfo.ref_mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                const std::vector<VkDescriptorSet> sets = { getMeshDescriptorSet(meshIndex) };
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    0,
                    sets.size(), sets.data(),
                    0, {}
                );

                vkCmdDrawIndexed(commandBuffer, renderMeshInfo.ref_mesh->meshIndexCount, 1, 0, 0, 0);
            }
        }
    }
}