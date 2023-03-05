#include "VulkanRenderer/Renderer.h"

#include <iostream>
#include <vector>
#include <set>
#include <cstring>
#include <limits>
#include <algorithm>
#include <chrono>
#include <thread>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#ifdef RELEASE_MODE_ON
#include <tracy/Tracy.hpp>
#endif

#include "VulkanRenderer/Settings/config.h"
#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Settings/ComputePipelineConfig.h"
#include "VulkanRenderer/Settings/VkLayersConfig.h"

#include "VulkanRenderer/Window/Window.h"

#include "VulkanRenderer/Queue/QueueFamilyIndices.h"
#include "VulkanRenderer/Queue/QueueFamilyHandles.h"

#include "VulkanRenderer/Swapchain/Swapchain.h"

#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/Command/CommandManager.h"

#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/Model/Attributes.h"
#include "VulkanRenderer/Model/Types/NormalPBR.h"
#include "VulkanRenderer/Model/Types/Skybox.h"
#include "VulkanRenderer/Model/Types/Light.h"

#include "VulkanRenderer/Descriptor/DescriptorPool.h"
#include "VulkanRenderer/Descriptor/DescriptorSetLayoutManager.h"
#include "VulkanRenderer/Descriptor/Types/UBO/UBO.h"
#include "VulkanRenderer/Descriptor/Types/UBO/UBOutils.h"
#include "VulkanRenderer/Descriptor/Types/DescriptorTypes.h"
#include "VulkanRenderer/Descriptor/DescriptorSets.h"

#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Texture/Type/NormalTexture.h"

#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"

#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/Camera/Types/Arcball.h" 

#include "VulkanRenderer/GUI/GUI.h"

#include "VulkanRenderer/Features/ShadowMap.h"
#include "VulkanRenderer/Image/ImageManager.h"

glm::fvec3 cameraPos = glm::fvec3(2.0f, 2.0f, 2.0f);


void Renderer::run()
{
#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    loadModels();

    // TODO: Improve this!
    // NUMBER OF VK_ATTACHMENT_LOAD_OP_CLEAR == CLEAR_VALUES
    m_clearValues.resize(2);
    m_clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[1].color = { 1.0f, 0.0f };

    m_clearValuesShadowMap.resize(2);
    m_clearValuesShadowMap[0].depthStencil.depth = 1.0f;
    m_clearValuesShadowMap[1].depthStencil.depth = 1.0f;
    m_clearValuesShadowMap[0].depthStencil.stencil = 0.0f;
    m_clearValuesShadowMap[1].depthStencil.stencil = 0.0f;

    if (m_models.size() == 0)
    {
        std::cout << "Nothing to render..\n";
        return;
    }

    initWindow();
    initVulkan();

    initComputations();
    doComputations();

    loadBRDFlut();

    uploadModels();

    m_camera = std::make_shared<Arcball>(
            m_window->get(),
            glm::fvec4(0.0f, 0.0f, 5.0f, 1.0f),
            glm::fvec4(0.0f),
            Config::FOV,
            (m_swapchain->getExtent().width / (float)m_swapchain->getExtent().height),
            Config::Z_NEAR,
            Config::Z_FAR
        );

    configureUserInputs();

    m_GUI = std::make_unique<GUI>(
        m_device->getPhysicalDevice(),
        m_device->getLogicalDevice(),
        m_vkInstance->get(),
        m_swapchain,
        m_qfIndices.graphicsFamily.value(),
        m_qfHandles.graphicsQueue,
        m_window
        );

    mainLoop();
    cleanup();
}

void Renderer::configureUserInputs()
{
    // Keyword and mouse settings
    m_isMouseInMotion = false;

    // If the user scrolls back, we'll zoom in the image.
    glfwSetWindowUserPointer(m_window->get(), m_camera.get());
    glfwSetScrollCallback(m_window->get(), scrollCallback);
}

void Renderer::loadModel(const size_t startI, const size_t chunckSize)
{
   const size_t endI = startI + chunckSize;

   for (size_t i = startI; i < endI; i++)
   {
       ModelInfo& modelInfo = m_modelsToLoadInfo[i];
      switch (modelInfo.type)
      {
         case ModelType::SKYBOX:
         {
            m_models.push_back(std::make_shared<Skybox>(modelInfo));
            m_skyboxModelIndex.push_back(m_models.size() - 1);
            m_skybox = std::dynamic_pointer_cast<Skybox>(m_models[m_skyboxModelIndex[0]]);
            break;
         }
         case ModelType::NORMAL_PBR:
         {
            m_models.push_back(std::make_shared<NormalPBR>(modelInfo));
            m_objectModelIndices.push_back(m_models.size() - 1);

            // TODO: delete this
            m_mainModelIndex = m_models.size() - 1;
            break;
         }
         case ModelType::LIGHT:
         {
             m_models.push_back(std::make_shared<Light>(modelInfo));
             m_lightModelIndices.push_back(m_models.size() - 1);

             // TODO: Improve this.
             if (modelInfo.lType == LightType::DIRECTIONAL_LIGHT)
                 m_directionalLightIndex = m_models.size() - 1;

            break;
         }
      }
   }
}

void Renderer::loadModels()
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    std::vector<std::thread> threads;

    const size_t maxThreadsCount = std::thread::hardware_concurrency() - 1;

    size_t chunckSize = ((m_modelsToLoadInfo.size() < maxThreadsCount) ? 1 : m_modelsToLoadInfo.size() / maxThreadsCount);

    const size_t threadsCount = ((m_modelsToLoadInfo.size() < maxThreadsCount) ? m_modelsToLoadInfo.size() : maxThreadsCount);

    for (size_t i = 0; i < threadsCount; i++)
    {
        if (i == threadsCount - 1 && maxThreadsCount < m_modelsToLoadInfo.size())
        {
            chunckSize = (m_modelsToLoadInfo.size() - (threadsCount * chunckSize));
        }
        threads.push_back(std::thread(&Renderer::loadModel, this, i, chunckSize));
    }
    for (auto& thread : threads)
        thread.join();
}


void Renderer::initWindow()
{
    uint32_t a = sizeof(float);
    m_window = std::make_shared<Window>(Config::RESOLUTION_W,Config::RESOLUTION_H,Config::WINDOW_TITLE);
}

void Renderer::createSyncObjects()
{
#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    m_imageAvailableSemaphores.resize(Config::MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(Config::MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(Config::MAX_FRAMES_IN_FLIGHT);


    //---------------------------Sync. Objects Info----------------------------
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Creates the fence in the signaled state, so that the first call to
    // vkWaitForFences() returns immediately since the fence is already signaled.
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;



    //---------------------Creation of Sync. Objects---------------------------

    for (size_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphore!");
        
        if (vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphore!");
        
        if (vkCreateFence(m_device->getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create fence!");
    }
}


void Renderer::createShadowMapRenderPass()
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    // - Attachments

    VkAttachmentDescription shadowMapAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        m_depthBuffer.getFormat(),
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        shadowMapAttachment
    );

    // Attachment references

    VkAttachmentReference shadowMapAttachmentRef{};
    AttachmentUtils::createAttachmentReference(0,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,shadowMapAttachmentRef);

    // Subpasses
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.pDepthStencilAttachment = &shadowMapAttachmentRef;

    m_renderPassShadowMap = RenderPass(m_device->getLogicalDevice(),{ shadowMapAttachment },{ subpass },{});
}

void Renderer::createSceneRenderPass()
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    // -Attachments
    
    // Color Attachment
    VkAttachmentDescription colorAttachment{};
    AttachmentUtils::createAttachmentDescription(
        m_swapchain->getImageFormat(),
        m_msaa.getSamplesCount(),
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorAttachment
    );

    // Depth Attachment
    VkAttachmentDescription depthAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        m_depthBuffer.getFormat(),
        m_msaa.getSamplesCount(),
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        // We don't care about storing the depth data, because it will not be
        // used after drawing has finished.
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        // Just like the color buffer, we don't care about the previous depth
        // contents.
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        depthAttachment
    );

    // Color Resolve Attachment(needed by MSAA)
    VkAttachmentDescription colorResolveAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        m_swapchain->getImageFormat(),
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        // Here is not
        // 'VK_IMAGE_LAYOUT_PRESENT_SRC_KHR'
        // because the GUI will be the last and the one
        // to present.
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorResolveAttachment
    );

    // Attachment references 
    
    VkAttachmentReference colorAttachmentRef{};
    AttachmentUtils::createAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, colorAttachmentRef);

    VkAttachmentReference depthAttachmentRef{};
    AttachmentUtils::createAttachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depthAttachmentRef);

    VkAttachmentReference colorResolveAttachmentRef{};
    AttachmentUtils::createAttachmentReference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, colorResolveAttachmentRef);


    // Subpasses
    std::vector<VkAttachmentReference> allAttachments = { colorAttachmentRef };
    VkSubpassDescription subPassDescript{};
    SubPassUtils::createSubPassDescription(
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        &colorAttachmentRef,
        &depthAttachmentRef,
        &colorResolveAttachmentRef,
        subPassDescript
    );

    // Subpass dependencies

    VkSubpassDependency dependency{};
    SubPassUtils::createSubPassDependency(
        // -Source parameters.
        //VK_SUBPASS_EXTERNAL means anything outside of a given render pass
        //scope. When used for srcSubpass it specifies anything that happened 
        //before the render pass. 
        VK_SUBPASS_EXTERNAL,
        // Operations that the subpass needs to wait on. 
        (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT),
        0,
        // -Destination parameters.
        0,
        (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT),
        (VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        (VkDependencyFlagBits)0,
        dependency
    );

    m_renderPass = RenderPass(
        m_device->getLogicalDevice(),
        { colorAttachment, depthAttachment, colorResolveAttachment },
        { subPassDescript },
        { dependency }
    );
}

/*
 * Uploads the data of each model to the gpu.
 */
void Renderer::uploadModels()
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    // First we upload the skybox because we need some dependencies from it for
    // the descriptor sets of the other models.

    m_skybox->upload(m_device->getPhysicalDevice(), m_device->getLogicalDevice(), m_qfHandles.graphicsQueue, m_commandPoolGraphics, Config::MAX_FRAMES_IN_FLIGHT);
    m_skybox->createDescriptorSets(m_device->getLogicalDevice(), m_graphicsPipelineSkybox.getDescriptorSetLayout(), nullptr, m_descriptorPoolGraphics);
    

    VkDescriptorSetLayout descriptorSetLayout;
    DescriptorSetInfo descriptorSetInfo = {
       &(m_skybox->getEnvMap()),
       &(m_skybox->getIrradianceMap()),
       &(*m_BRDFlut),
       &(m_shadowMap->getShadowMapView()),
       &(m_shadowMap->getSampler())
    };

    for (auto& model : m_models)
    {
        auto type = model->getType();
        if (type == ModelType::SKYBOX)
            continue;

        model->upload(m_device->getPhysicalDevice(), m_device->getLogicalDevice(), m_qfHandles.graphicsQueue, m_commandPoolGraphics, Config::MAX_FRAMES_IN_FLIGHT);

        // Descriptor Sets
        if (type == ModelType::NORMAL_PBR)
        {
            descriptorSetLayout = (m_graphicsPipelinePBR.getDescriptorSetLayout());
        }
        else if (type == ModelType::LIGHT)
        {
            descriptorSetLayout = (m_graphicsPipelineLight.getDescriptorSetLayout());
        }
        else
        {
            std::cout << "TODO";
        }
        model->createDescriptorSets(m_device->getLogicalDevice(), descriptorSetLayout, &descriptorSetInfo, m_descriptorPoolGraphics);
    }
}

void Renderer::createPipelines()
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    // --------------------------------GRAPHICS---------------------------------
    {
        // - Models
        m_graphicsPipelineSkybox = Graphics(
            m_device->getLogicalDevice(),
            GraphicsPipelineType::SKYBOX,
            m_swapchain->getExtent(),
            m_renderPass.get(),
            { { shaderType::VERTEX,"skybox"},{shaderType::FRAGMENT,"skybox"} },
            m_msaa.getSamplesCount(),
            Attributes::SKYBOX::getBindingDescription(),
            Attributes::SKYBOX::getAttributeDescriptions(),
            & m_skyboxModelIndex,
            GRAPHICS_PIPELINE::SKYBOX::UBOS_INFO,
            GRAPHICS_PIPELINE::SKYBOX::SAMPLERS_INFO
        );

        m_graphicsPipelinePBR = Graphics(
            m_device->getLogicalDevice(),
            GraphicsPipelineType::PBR,
            m_swapchain->getExtent(),
            m_renderPass.get(),
            { { shaderType::VERTEX,"scene"},{shaderType::FRAGMENT,"scene"} },
            m_msaa.getSamplesCount(),
            Attributes::PBR::getBindingDescription(),
            Attributes::PBR::getAttributeDescriptions(),
            & m_objectModelIndices,
            GRAPHICS_PIPELINE::PBR::UBOS_INFO,
            GRAPHICS_PIPELINE::PBR::SAMPLERS_INFO
        );

        m_graphicsPipelineLight = Graphics(
            m_device->getLogicalDevice(),
            GraphicsPipelineType::LIGHT,
            m_swapchain->getExtent(),
            m_renderPass.get(),
            { { shaderType::VERTEX,"light"},{shaderType::FRAGMENT,"light"} },
            m_msaa.getSamplesCount(),
            Attributes::LIGHT::getBindingDescription(),
            Attributes::LIGHT::getAttributeDescriptions(),
            & m_lightModelIndices,
            GRAPHICS_PIPELINE::LIGHT::UBOS_INFO,
            GRAPHICS_PIPELINE::LIGHT::SAMPLERS_INFO
        );

        // - Features
        m_graphicsPipelineShadowMap = Graphics(
            m_device->getLogicalDevice(),
            GraphicsPipelineType::SHADOWMAP,
            m_swapchain->getExtent(),
            m_renderPassShadowMap.get(),
            { {shaderType::VERTEX, "shadowMap"} },
            VK_SAMPLE_COUNT_1_BIT,
            Attributes::SHADOWMAP::getBindingDescription(),
            Attributes::SHADOWMAP::getAttributeDescriptions(),
            & m_objectModelIndices,
            GRAPHICS_PIPELINE::SHADOWMAP::UBOS_INFO,
            {}
        );
    }

}

void Renderer::createCommandPools()
{
    // Graphics Command Pool
    {
        m_commandPoolGraphics = std::make_shared<CommandPool>(
                m_device->getLogicalDevice(),
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                m_qfIndices.graphicsFamily.value()
            );

        m_commandPoolGraphics->allocCommandBuffers(Config::MAX_FRAMES_IN_FLIGHT);
    }

    // Compute Command Pool
    {
        m_commandPoolCompute = std::make_shared<CommandPool>(
                m_device->getLogicalDevice(),
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                m_qfIndices.computeFamily.value()
            );

        m_commandPoolCompute->allocCommandBuffers(1);
    }

    // Features Command Pool
    {
        m_shadowMap->createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_qfIndices.graphicsFamily.value());

        m_shadowMap->allocCommandBuffers(Config::MAX_FRAMES_IN_FLIGHT);
    }

}

void Renderer::initComputations()
{
    m_BRDFcomp = Computation(
        m_device->getPhysicalDevice(),
        m_device->getLogicalDevice(),
        "BRDF",
        sizeof(float),
        2 * sizeof(float) * Config::BRDF_HEIGHT * Config::BRDF_WIDTH,
        m_qfIndices,
        m_descriptorPoolComputations,
        COMPUTE_PIPELINE::BRDF::BUFFERS_INFO
    );
}

void Renderer::initVulkan()
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    m_vkInstance = std::make_unique<VKinstance>(Config::WINDOW_TITLE);

    m_window->createSurface(m_vkInstance->get());

    m_device = std::make_unique<Device>(m_vkInstance->get(), m_qfIndices, m_window->getSurface());

    m_qfHandles.setQueueHandles(m_device->getLogicalDevice(), m_qfIndices);

    m_swapchain = std::make_unique<Swapchain>(m_device->getPhysicalDevice(), m_device->getLogicalDevice(), m_window, m_device->getSupportedProperties());

 
    //------------------------------Descriptor Pools----------------------------
    m_descriptorPoolGraphics = DescriptorPool(
        m_device->getLogicalDevice(),
        {
            // TODO: Make the size more precise.
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1500/*,static_cast<uint32_t> (m_models.size() * Config::MAX_FRAMES_IN_FLIGHT)*/},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1500/*,static_cast<uint32_t> (m_models.size() * Config::MAX_FRAMES_IN_FLIGHT)*/}
        },
        1500
        /*m_models.size() * Config::MAX_FRAMES_IN_FLIGHT*/
    );

    m_descriptorPoolComputations = DescriptorPool(
        m_device->getLogicalDevice(),
        {
           {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2}
        },
        1 // just for the BRDF(for now..)
    );


    // -------------------------------Main Features------------------------------
    m_msaa = MSAA(m_device->getPhysicalDevice(), m_device->getLogicalDevice(), m_swapchain->getExtent(), m_swapchain->getImageFormat());

    m_depthBuffer = DepthBuffer(m_device->getPhysicalDevice(),m_device->getLogicalDevice(),m_swapchain->getExtent(), m_msaa.getSamplesCount());

 
    //----------------------------------RenderPasses------------------------------
    createShadowMapRenderPass();
    createSceneRenderPass();

    //----------------------------------Pipelines-------------------------------
    createPipelines();


    //-----------------------------Secondary Features---------------------------
    //(these features they are not used by all the pipelines and need
    //dependencies)
    m_shadowMap = std::make_shared<ShadowMap<Attributes::PBR::Vertex>>(
            m_device->getPhysicalDevice(),
            m_device->getLogicalDevice(),
            m_swapchain->getExtent().width,
            m_swapchain->getExtent().height,
            m_depthBuffer.getFormat(),
            m_graphicsPipelineShadowMap.getDescriptorSetLayout(),
            Config::MAX_FRAMES_IN_FLIGHT,
            &(std::dynamic_pointer_cast<NormalPBR>(m_models[m_mainModelIndex])->getMeshes())
        );

    //----------------------------------Framebuffers----------------------------

    m_swapchain->createFramebuffers(m_renderPass.get(), m_depthBuffer, m_msaa);
    m_shadowMap->createFramebuffer(m_renderPassShadowMap.get(), m_swapchain->getImageCount());



    //--------------------------------------------------------------------------
    createCommandPools();

    createSyncObjects();
}


void Renderer::recordCommandBuffer(
    const VkFramebuffer& framebuffer,
    const RenderPass& renderPass,
    const VkExtent2D& extent,
    const std::vector<Graphics>& graphicsPipelines,
    const uint32_t currentFrame,
    const VkCommandBuffer& commandBuffer,
    const std::vector<VkClearValue>& clearValues,
    const std::shared_ptr<CommandPool>& commandPool
) {
    // Resets the command buffer to be able to be recorded.
    commandPool->resetCommandBuffer(currentFrame);

    // Specifies some details about the usage of this specific command buffer.
    commandPool->beginCommandBuffer(0, commandBuffer);

    //--------------------------------RenderPass-----------------------------
    renderPass.begin(framebuffer, extent, clearValues, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);


        //---------------------------------CMDs-------------------------------
        for (const auto& graphicsPipeline : graphicsPipelines)
        {
            CommandManager::STATE::bindPipeline(graphicsPipeline.get(), PipelineType::GRAPHICS, commandBuffer);
            // Set Dynamic States
            CommandManager::STATE::setViewport(0.0f, 0.0f, extent, 0.0f, 1.0f, 0, 1, commandBuffer);
            CommandManager::STATE::setScissor({ 0, 0 }, extent, 0, 1, commandBuffer);

            if (graphicsPipeline.getGraphicsPipelineType() ==GraphicsPipelineType::SHADOWMAP) 
            {
                m_shadowMap->bindData(graphicsPipeline,commandBuffer,currentFrame);
                continue;
            }

            for (const size_t& i : graphicsPipeline.getModelIndices())
            {
                const auto& model = m_models[i];
                model->bindData(graphicsPipeline,commandBuffer,currentFrame);

            }
        }
        renderPass.end(commandBuffer);

    commandPool->endCommandBuffer(commandBuffer);
}


void Renderer::drawFrame(uint8_t& currentFrame)
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    // Waits until the previous frame has finished.
   //    - 2 param. -> FenceCount.
   //    - 4 param. -> waitAll.
   //    - 5 param. -> timeOut.
    vkWaitForFences(m_device->getLogicalDevice(), 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // After waiting, we need to manually reset the fence.
    vkResetFences(m_device->getLogicalDevice(), 1, &m_inFlightFences[currentFrame]);


    //------------------------Updates uniform buffer----------------------------

    // First we update the shadow map since the other models of the scene have dependencies with it.
    // Shadow Map
    if (m_directionalLightIndex.has_value())
    {
        auto pLight = std::dynamic_pointer_cast<Light>(m_models[m_directionalLightIndex.value()]);
        // TODO: improve this
        auto pMainModel = std::dynamic_pointer_cast<NormalPBR>(m_models[m_mainModelIndex]);

        m_shadowMap->updateUBO(
            // TODO: make it for more than 1 model
            pMainModel->getModelM(),
            pLight->getPos(),
            pLight->getTargetPos(),
            1.0,
            Config::Z_NEAR,
            Config::Z_FAR,
            currentFrame
        );
    }


    UBOinfo uboInfo = {
      m_camera->getPos(),
      m_camera->getViewM(),
      m_camera->getProjectionM(),
      m_shadowMap->getLightSpace(),
      m_lightModelIndices.size(),
      m_swapchain->getExtent()
    };

    // Scene
    for (auto& model : m_models)
    {
        model->updateUBO(m_device->getLogicalDevice(), currentFrame, uboInfo);

        if (auto pModel = std::dynamic_pointer_cast<NormalPBR>(model))
        {
            pModel->updateUBOlights(m_device->getLogicalDevice(), m_lightModelIndices, m_models, currentFrame);
        }
    }

    //--------------------Acquires an image from the swapchain------------------

    const uint32_t imageIndex = m_swapchain->getNextImageIndex(m_imageAvailableSemaphores[currentFrame]);


    //---------------------Records all the command buffer-----------------------    

    // Shadow Mapping
    recordCommandBuffer(
        m_shadowMap->getFramebuffer(imageIndex),
        m_renderPassShadowMap,
        m_swapchain->getExtent(),
        {m_graphicsPipelineShadowMap},
        currentFrame,
        m_shadowMap->getCommandBuffer(currentFrame),
        m_clearValuesShadowMap,
        m_shadowMap->getCommandPool()
    );

    // Scene
    recordCommandBuffer(
        m_swapchain->getFramebuffer(imageIndex),
        m_renderPass,
        m_swapchain->getExtent(),
        { m_graphicsPipelineLight,m_graphicsPipelinePBR, m_graphicsPipelineSkybox },
        currentFrame,
        m_commandPoolGraphics->getCommandBuffer(currentFrame),
        m_clearValues,
        m_commandPoolGraphics
    );

    // GUI
    m_GUI->recordCommandBuffer(currentFrame, imageIndex, m_clearValues);

    //----------------------Submits the command buffer -------------------------

    std::vector<VkCommandBuffer> commandBuffersToSubmit = { m_shadowMap->getCommandBuffer(currentFrame), m_commandPoolGraphics->getCommandBuffer(currentFrame),m_GUI->getCommandBuffer(currentFrame) };

    std::vector<VkSemaphore> waitSemaphores = { m_imageAvailableSemaphores[currentFrame] };
    std::vector<VkSemaphore> signalSemaphores = { m_renderFinishedSemaphores[currentFrame] };

    m_commandPoolGraphics->submitCommandBuffer(
        m_qfHandles.graphicsQueue, 
        commandBuffersToSubmit,
        false,
        waitSemaphores,
        (VkPipelineStageFlags)(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT),
        signalSemaphores,
        m_inFlightFences[currentFrame]
    );


    //-------------------Presentation of the swapchain image--------------------
    
    m_swapchain->presentImage(imageIndex, signalSemaphores, m_qfHandles.presentQueue);

    // Updates the frame
    currentFrame = (currentFrame + 1) % Config::MAX_FRAMES_IN_FLIGHT;
}


void Renderer::scrollCallback(GLFWwindow* window,double xoffset,double yoffset) 
{
#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));

    float actualFOV = camera->getFOV();
    float newFOV = actualFOV + yoffset * -1.0f;

    camera->setFOV(newFOV);
}

void Renderer::handleInput()
{
#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    m_window->pollEvents();
    // Avoids any input when we're touching the IMGUI.
    if (m_GUI->isCursorPositionInGUI())
        return;

    if (m_camera->getType() == CameraType::ARCBALL)
    {
        if (glfwGetMouseButton(m_window->get(), GLFW_MOUSE_BUTTON_LEFT) ==GLFW_PRESS) 
        {
            auto pCamera = std::dynamic_pointer_cast<Arcball>(m_camera);
            if (!m_isMouseInMotion)
            {
                pCamera->saveCursorPos();
                m_isMouseInMotion = true;
            }
            else 
            {
                // TODO: Make it dynamic.
                glm::mat4 newRot = glm::mat4(1.0);

                pCamera->updateCameraPos(newRot);
            }
        }
        else
        {
            m_isMouseInMotion = false;
        }
    }
}

void Renderer::mainLoop()
{
    // Tells us in which frame we are,
    // between 1 <= frame <= MAX_FRAMES_IN_FLIGHT

    uint8_t currentFrame = 0;
    while (m_window->isWindowClosed() == false)
    {
        handleInput();
        m_GUI->draw(m_models, m_camera, m_objectModelIndices, m_lightModelIndices);
        drawFrame(currentFrame);
    }
    vkDeviceWaitIdle(m_device->getLogicalDevice());
}

void Renderer::loadBRDFlut()
{
    const uint32_t bufferSize = (2 * sizeof(float) * Config::BRDF_WIDTH * Config::BRDF_HEIGHT);
    float* lutData = new float[bufferSize];
   
    m_BRDFcomp.downloadData(0, (uint8_t*)lutData, bufferSize);

    gli::texture lutTexture = gli::texture2d(
        gli::FORMAT_RG16_SFLOAT_PACK16,
        gli::extent2d(Config::BRDF_WIDTH, Config::BRDF_HEIGHT),
        1
    );

    const float* data = lutData;
    for (int y = 0; y < Config::BRDF_HEIGHT; y++)
    {
        for (int x = 0; x < Config::BRDF_WIDTH; x++)
        {
            const int ofs = y * Config::BRDF_HEIGHT + x;
            const gli::vec2 value(data[ofs * 2 + 0], data[ofs * 2 + 1]);
            const gli::texture::extent_type uv = { x, y, 0 };

            lutTexture.store<glm::uint32>(uv, 0, 0, 0, gli::packHalf2x16(value));
        }
    }

    std::string pathToTexture = (std::string(SKYBOX_DIR) + m_skybox->getTextureFolderName() + "/" + "BRDFlut.ktx");

    gli::save_ktx(lutTexture, pathToTexture);

    TextureToLoadInfo info = { pathToTexture,VK_FORMAT_R16G16_SFLOAT,4 };

    m_BRDFlut = std::make_shared<NormalTexture>(
        m_device->getPhysicalDevice(),
        m_device->getLogicalDevice(),
        info,
        VK_SAMPLE_COUNT_1_BIT,
        m_commandPoolGraphics,
        m_qfHandles.graphicsQueue,
        UsageType::BRDF
        );

}

void Renderer::doComputations()
{
    std::vector<Computation> computations = {m_BRDFcomp};

    std::cout << "Doing computations.\n";

    const VkCommandBuffer& commandBuffer = (m_commandPoolCompute->getCommandBuffer(0));

    for (auto& computation : computations)
    {
        // Resets the command buffer to be able to be recorded.
        m_commandPoolCompute->resetCommandBuffer(0);
        // Specifies some details about the usage of this specific command buffer.
        m_commandPoolCompute->beginCommandBuffer(0, commandBuffer);

        computation.execute(commandBuffer);

        // To make sure that the buffer is ready to be accessed.
        VkMemoryBarrier readBarrier = {
           VK_STRUCTURE_TYPE_MEMORY_BARRIER,
           nullptr,
           // Specifies that we'll wait for the shader to finish writing to the
           // buffer.
           VK_ACCESS_SHADER_WRITE_BIT,
           VK_ACCESS_HOST_READ_BIT
        };

        CommandManager::SYNCHRONIZATION::recordPipelineBarrier(
            // Specifies that we'll wait for the Compute Queue to finish the
            // execution of the tasks.
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            0,
            commandBuffer,
            { readBarrier },
            {},
            {}
        );

        m_commandPoolCompute->endCommandBuffer(commandBuffer);

        m_commandPoolCompute->submitCommandBuffer(m_qfHandles.computeQueue, { commandBuffer }, true);
    }

    std::cout << "All the computations have been completed.\n";

}

void Renderer::destroySyncObjects()
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    for (size_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_device->getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device->getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_device->getLogicalDevice(), m_inFlightFences[i], nullptr);
    }
}

void Renderer::cleanup()
{
#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    // MSAA
    m_msaa.destroy();

    // DepthBuffer
    m_depthBuffer.destroy();

    // ImGui
    m_GUI->destroy();

    // Swapchain
    m_swapchain->destroy();

    // Graphics Pipelines
    m_graphicsPipelinePBR.destroy();
    m_graphicsPipelineSkybox.destroy();
    m_graphicsPipelineLight.destroy();
    m_graphicsPipelineShadowMap.destroy();

    // Computations
    m_BRDFcomp.destroy();
    m_BRDFlut->destroy();

    // Renderpass
    m_renderPass.destroy();
    m_renderPassShadowMap.destroy();

    // Models -> Buffers, Memories and Textures.
    m_shadowMap->destroy();
    for (auto& model : m_models)
        model->destroy(m_device->getLogicalDevice());
   
    // Descriptor Pool
    m_descriptorPoolGraphics.destroy();
    m_descriptorPoolComputations.destroy();

    // Sync objects
    destroySyncObjects();

    // Command Pools
    if (m_commandPoolGraphics) m_commandPoolGraphics->destroy();
    if (m_commandPoolCompute)  m_commandPoolCompute->destroy();

    // Logical Device
    vkDestroyDevice(m_device->getLogicalDevice(), nullptr);

    // Window Surface
    m_window->destroySurface(m_vkInstance->get());

    // Vulkan's instance
    m_vkInstance->destroy();

    // GLFW
    m_window->destroy();
}

void Renderer::addSkybox(const std::string& name, const std::string& textureFolderName)
{
    m_modelsToLoadInfo.push_back({
         ModelType::SKYBOX,name, textureFolderName,
         glm::fvec3(0.0f),
         glm::fvec3(0.0f),
         glm::fvec3(0.0f),
         glm::fvec3(0.0f),
         LightType::NONE,
         glm::fvec3(0.0f),
         0.0f,
         0.0f
        });
}

void Renderer::addObjectPBR(const std::string& name, const std::string& modelFileName,
    const glm::fvec3& pos,
    const glm::fvec3& rot,
    const glm::fvec3& size ) 
{
    m_modelsToLoadInfo.push_back({
         ModelType::NORMAL_PBR,
         name,
         modelFileName,
         glm::fvec3(0.0f),
         pos,
         rot,
         size,
         LightType::NONE,
         glm::fvec3(0.0f),
         0.0f,
         0.0f
        });
}

void Renderer::addDirectionalLight(
    const std::string& name,
    const std::string& modelFileName,
    const glm::fvec3& color,
    const glm::fvec3& pos,
    const glm::fvec3& endPos,
    const glm::fvec3& size
) {
    if (!m_directionalLightIndex.has_value())
    {
        m_modelsToLoadInfo.push_back({
            ModelType::LIGHT,
            name,
            modelFileName,
            color,
            pos,
            glm::fvec3(0.0f),
            size,
            LightType::DIRECTIONAL_LIGHT,
            endPos,
            0.0f,
            0.0f
            });
    }
    else
    {
        throw std::runtime_error(
            "Just one directional light per scene is allowed to be added."
        );
    }
}

void Renderer::addPointLight(
    const std::string& name,
    const std::string& modelFileName,
    const glm::fvec3& color,
    const glm::fvec3& pos,
    const glm::fvec3& size,
    const float attenuation,
    const float radius
) {
    m_modelsToLoadInfo.push_back({
          ModelType::LIGHT,
          name,
          modelFileName,
          color,
          pos,
          glm::fvec3(0.0f),
          size,
          LightType::POINT_LIGHT,
          glm::fvec3(0.0f),
          attenuation,
          radius
        });
}

void Renderer::addSpotLight(
    const std::string& name,
    const std::string& modelFileName,
    const glm::fvec3& color,
    const glm::fvec3& pos,
    const glm::fvec3& endPos,
    const glm::fvec3& rot,
    const glm::fvec3& size,
    const float attenuation,
    const float radius
) {
    m_modelsToLoadInfo.push_back({
          ModelType::LIGHT,
          name,
          modelFileName,
          color,
          pos,
          rot,
          size,
          LightType::SPOT_LIGHT,
          endPos,
          attenuation,
          radius
        });
}