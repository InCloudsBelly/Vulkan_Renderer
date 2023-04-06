#include "VulkanRenderer/Scene/ForwardPBR.h"

#include <iostream>

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
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

void ForwardPBRPass::createDescriptorSet(const std::shared_ptr<Model> modelPtr, const VkDescriptorSetLayout& layout, std::vector<VkDescriptorImageInfo*> additionalImages, const std::vector<DescriptorInfo>& descriptorInfos)
{
    for (uint32_t meshIndex : modelPtr->getMeshIndices())
    {
        //create DescriptorSet PerMesh
        DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), layout, &m_descriptorSetsMap[meshIndex]);

        RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];
        std::vector<std::shared_ptr<TextureBase>> materialTextures;
        if (renderMeshInfo.ref_material != nullptr)
        {
            materialTextures = {
                renderMeshInfo.ref_material->colorTexture,
                renderMeshInfo.ref_material->metallic_RoughnessTexture,
                renderMeshInfo.ref_material->emissiveTexture,
                renderMeshInfo.ref_material->AOTexture,
                renderMeshInfo.ref_material->normalTexture
            };
        }
        else
            materialTextures = {};

        DescriptorManager::createDescriptorSet(
            descriptorInfos,
            materialTextures,
            additionalImages,
            m_ubosMap[meshIndex],
            &m_descriptorSetsMap[meshIndex]
        );
    }
}



void ForwardPBRPass::createPipelines()
{
    VkSampleCountFlagBits msaaSamplesCount = getRendererPointer()->getMSAAInfo().msaa_sampleCount;

    m_graphicsPipelineSkybox = Graphics(
        GraphicsPipelineType::SKYBOX,
        m_extent,
        m_renderPass,
        { {shaderType::VERTEX, "skybox"}, {shaderType::FRAGMENT, "skybox"} },
        msaaSamplesCount,
        Attributes::SKYBOX::getBindingDescription(),
        Attributes::SKYBOX::getAttributeDescriptions(),
        GRAPHICS_PIPELINE::SKYBOX::DESCRIPTORS_INFO,
        {}
    );

    m_graphicsPipelinePBR = Graphics(
        GraphicsPipelineType::PBR,
        m_extent,
        m_renderPass,
        { {shaderType::VERTEX, "scene"}, {shaderType::FRAGMENT, "scene"} },
        msaaSamplesCount,
        Attributes::PBR::getBindingDescription(),
        Attributes::PBR::getAttributeDescriptions(),
        //Models asssocciated with this graphics pipeline.
        GRAPHICS_PIPELINE::PBR::DESCRIPTORS_INFO,
        {}
    );

    m_graphicsPipelineLight = Graphics(
        GraphicsPipelineType::LIGHT,
        m_extent,
        m_renderPass,
        { {shaderType::VERTEX, "light"},{shaderType::FRAGMENT,"light"} },
        msaaSamplesCount,
        Attributes::LIGHT::getBindingDescription(),
        Attributes::LIGHT::getAttributeDescriptions(),
        // Models assocciated with this graphics pipeline.
        GRAPHICS_PIPELINE::LIGHT::DESCRIPTORS_INFO,
        {}
    );
}





const Graphics& ForwardPBRPass::getPBRpipeline() const
{
    return m_graphicsPipelinePBR;
}

const Graphics& ForwardPBRPass::getSkyboxPipeline() const
{
    return m_graphicsPipelineSkybox;
}

const Graphics& ForwardPBRPass::getLightPipeline() const
{
    return m_graphicsPipelineLight;
}

void ForwardPBRPass::updateUBO(
    const VkExtent2D& extent,
    const uint32_t& currentFrame
) {
    m_shadowMap->updateUBO();
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

    for (auto ptr : getRenderResource()->m_lightModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            DescriptorTypes::UniformBufferObject::Light newUBO;
            newUBO.model = ptr->getModelMatrix();

            newUBO.view = uboInfo.view;
            newUBO.proj = uboInfo.proj;
            newUBO.lightColor = glm::fvec4(1.0f);

            void* data;
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0], &data);
            memcpy(data, &newUBO, sizeof(newUBO));
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0]);
        }
    }
 
    auto skybox = getRenderResource()->m_skybox;
    for (uint32_t meshIndex : skybox->getMeshIndices())
    {
        DescriptorTypes::UniformBufferObject::Skybox newUBO;

        newUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(uboInfo.cameraPos));
        newUBO.view = uboInfo.view;
        newUBO.proj = MathUtils::getUpdatedProjMatrix(glm::radians(75.0f), uboInfo.extent.width / (float)uboInfo.extent.height, 0.01f, 40.0f);

        void* data;
        vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0], &data);
        memcpy(data, &newUBO, sizeof(newUBO));
        vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0]);
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
    drawPipeline(commandBuffer, m_graphicsPipelinePBR, getRenderResource()->m_normalModels);
    drawPipeline(commandBuffer, m_graphicsPipelineLight, getRenderResource()->m_lightModels);
    drawPipeline(commandBuffer, m_graphicsPipelineSkybox, { getRenderResource()->m_skybox });
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

    //Light Models
    for (auto ptr : getRenderResource()->m_lightModels)
    {
        std::vector<size_t> uboSizeInfos = { sizeof(DescriptorTypes::UniformBufferObject::Light) };
        createUniformBuffer(ptr, uboSizeInfos);
    }

    //Skybox
    auto skybox = getRenderResource()->m_skybox;
    std::vector<size_t> uboSizeInfos = { sizeof(DescriptorTypes::UniformBufferObject::Skybox) };
    createUniformBuffer(skybox, uboSizeInfos);
}

void ForwardPBRPass::createDescriptorSets()
{
    std::vector<VkDescriptorImageInfo*>  additionalImage = {
        &(getRenderResource()->m_IBLResource.irradiance->getDescriptorImageInfo()) ,
        &(getRenderResource()->m_IBLResource.brdfLUT->getDescriptorImageInfo()),
        &(getRenderResource()->m_IBLResource.prefiltered_Env->getDescriptorImageInfo()),
        &(m_shadowMap->get()->getDescriptorImageInfo())
    };

    for (auto ptr : getRenderResource()->m_normalModels)
        createDescriptorSet(ptr, m_graphicsPipelinePBR.getDescriptorSetLayout(), additionalImage, GRAPHICS_PIPELINE::PBR::DESCRIPTORS_INFO);
   
    for (auto ptr : getRenderResource()->m_lightModels)
        createDescriptorSet(ptr, m_graphicsPipelineLight.getDescriptorSetLayout(), { &getRenderResource()->m_defaultTexture->getDescriptorImageInfo() }, GRAPHICS_PIPELINE::LIGHT::DESCRIPTORS_INFO);

    auto skybox = getRenderResource()->m_skybox;
    createDescriptorSet(skybox, m_graphicsPipelineSkybox.getDescriptorSetLayout(), { &getRenderResource()->m_skyboxCubeMap->getDescriptorImageInfo() }, GRAPHICS_PIPELINE::SKYBOX::DESCRIPTORS_INFO);
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

    m_graphicsPipelinePBR.destroy();
    m_graphicsPipelineSkybox.destroy();
    m_graphicsPipelineLight.destroy();

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

void ForwardPBRPass::drawPipeline(const VkCommandBuffer& commandBuffer, const Pipeline& pipeline, std::vector<std::shared_ptr<Model>> models)
{

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get());

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
                    pipeline.getPipelineLayout(),
                    0,
                    sets.size(), sets.data(),
                    0, {}
                );

                vkCmdDrawIndexed(commandBuffer, renderMeshInfo.ref_mesh->meshIndexCount, 1, 0, 0, 0);
            }
        }
    }
}
