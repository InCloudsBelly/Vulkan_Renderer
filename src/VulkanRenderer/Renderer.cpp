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

#include "VulkanRenderer/Descriptor/DescriptorTypes.h"
#include "VulkanRenderer/Descriptor/DescriptorSets.h"

#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Texture/Texture.h"

#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"

#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/Camera/Types/Arcball.h" 

#include "VulkanRenderer/GUI/GUI.h"

#include "VulkanRenderer/Features/ShadowMap.h"

glm::fvec3 cameraPos = glm::fvec3(2.0f, 2.0f, 2.0f);

Renderer* g_RendererSingleton = nullptr;

void Renderer::run()
{
#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    g_RendererSingleton = this;

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

    initWindow();
    initVulkan();

    doComputations();

    m_scene.upload(
        m_device->getPhysicalDevice(),
        m_qfHandles.graphicsQueue,
        m_commandPoolForGraphics,
        m_descriptorPoolForGraphics,
        m_shadowMap
    );

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


void Renderer::createCommandPools()
{
    

    // Features Command Pool
    {
        m_shadowMap->createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_qfIndices.graphicsFamily.value());

        m_shadowMap->allocCommandBuffers(Config::MAX_FRAMES_IN_FLIGHT);
    }

}


void Renderer::initVulkan()
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    m_vkInstance = std::make_unique<VKinstance>(Config::WINDOW_TITLE);

    m_window->createSurface(m_vkInstance->get());


    VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
    enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;

    m_device = std::make_unique<Device>(m_vkInstance->get(), m_qfIndices, m_window->getSurface(), &enabledBufferDeviceAddresFeatures);

    m_qfHandles.setQueueHandles(m_device->getLogicalDevice(), m_qfIndices);

    
    
    vhMemCreateVMAAllocator(m_vkInstance->get(), m_device->getPhysicalDevice(), m_device->getLogicalDevice(), m_vmaAllocator);




    m_swapchain = std::make_unique<Swapchain>(m_device->getPhysicalDevice(), m_device->getLogicalDevice(), m_window, m_device->getSupportedProperties());

 
    //------------------------------Descriptor Pools----------------------------
    m_descriptorPoolForGraphics = DescriptorPool(
        m_device->getLogicalDevice(),
        {
            // framesInFlight * #allmeshes
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,static_cast<uint32_t>(m_modelsToLoadInfo.size()) * Config::MAX_FRAMES_IN_FLIGHT * 100 },
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_modelsToLoadInfo.size()) * Config::MAX_FRAMES_IN_FLIGHT * 100 }
        },
        m_modelsToLoadInfo.size()* Config::MAX_FRAMES_IN_FLIGHT * 100
    );

    m_descriptorPoolForComputations = DescriptorPool(
        m_device->getLogicalDevice(),
        {
           {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2}
        },
        1 // just for the BRDF(for now..)
    );

    //------------------------------Graphics Pools----------------------------
    // Graphics Command Pool
    {
        m_commandPoolForGraphics = std::make_shared<CommandPool>(
            m_device->getLogicalDevice(),
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            m_qfIndices.graphicsFamily.value()
            );

        m_commandPoolForGraphics->allocCommandBuffers(Config::MAX_FRAMES_IN_FLIGHT);
    }

    // Compute Command Pool
    {
        m_commandPoolForCompute = std::make_shared<CommandPool>(
            m_device->getLogicalDevice(),
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            m_qfIndices.computeFamily.value()
            );

        m_commandPoolForCompute->allocCommandBuffers(1);
    }


    // -------------------------------Main Features------------------------------
    m_msaa = MSAA(m_swapchain->getExtent(), m_swapchain->getImageFormat());

    m_depthBuffer = DepthBuffer(m_swapchain->getExtent(), m_msaa.getSamplesCount());

 
    m_scene = Scene(
        m_device->getLogicalDevice(),
        m_swapchain->getImageFormat(),
        m_swapchain->getExtent(),
        m_msaa.getSamplesCount(),
        m_depthBuffer.getFormat(),
        m_modelsToLoadInfo,
        // Parameters needed by the computations.
        m_device->getPhysicalDevice(),
        m_qfIndices,
        m_descriptorPoolForComputations,
        m_vmaAllocator
    );


    //-----------------------------Secondary Features---------------------------
    //(these features they are not used by all the pipelines and need dependencies)

    const VkExtent2D shadowExtent = { 2 * m_swapchain->getExtent().height, 2 * m_swapchain->getExtent().width };
    m_shadowMap = std::make_shared<ShadowMap<Attributes::PBR::Vertex>>(
            shadowExtent,
            m_swapchain->getImageCount(),
            m_depthBuffer.getFormat(),
            Config::MAX_FRAMES_IN_FLIGHT,
            &(std::dynamic_pointer_cast<NormalPBR>(m_scene.getMainModel())->getMeshes()),
            m_scene.getObjectModelIndices()
        );

    //----------------------------------Framebuffer----------------------------
    m_swapchain->createFramebuffers(m_scene.getRenderPass(), m_depthBuffer, m_msaa);

    //--------------------------------------------------------------------------
    createCommandPools();

    createSyncObjects();
}


void Renderer::recordCommandBuffer(
    const VkFramebuffer& framebuffer,
    const RenderPass& renderPass,
    const VkExtent2D& extent,
    const std::vector<const Graphics*>& graphicsPipelines,
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
        for (auto graphicsPipeline : graphicsPipelines)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->get());

            // Set Dynamic States
            VkViewport viewport{ 0.0f, 0.0f, extent.width,extent.height, 0.0f, 1.0f};
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            
            VkRect2D scissor{ {0,0}, {extent.width,extent.height} };
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            if (graphicsPipeline->getGraphicsPipelineType() ==GraphicsPipelineType::SHADOWMAP) 
            {
                for (uint32_t i = 0; i < m_scene.getObjectModelIndices().size(); i++)
                {
                    auto& model = m_scene.getModel(m_scene.getObjectModelIndices()[i]);
                    if (model->isHidden() == false)
                    {
                        m_shadowMap->bindData(&(std::dynamic_pointer_cast<NormalPBR>(model)->getMeshes()),i, commandBuffer, currentFrame);
                    }
                }
                continue;
            }

            for (auto i : graphicsPipeline->getModelIndices())
            {
                auto& model = m_scene.getModel(i);
                
                if (model->isHidden() == false)
                {
                    model->bindData(graphicsPipeline, commandBuffer, currentFrame);
                }
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
    {
        auto pLight = std::dynamic_pointer_cast<Light>(m_scene.getDirectionalLight());

      /*  auto pMainModel = std::dynamic_pointer_cast<NormalPBR>(m_scene.getMainModel());*/

        for (uint32_t i = 0;  i < m_scene.getObjectModelIndices().size(); i++)
        {
            m_shadowMap->updateUBO(
                // TODO: make it for more than 1 model
                (std::dynamic_pointer_cast<NormalPBR>(m_scene.getModel(m_scene.getObjectModelIndices()[i])))->getModelM(),
                pLight->getPos(),
                pLight->getTargetPos(),
                1.0,
                Config::Z_NEAR_SHADOW,
                Config::Z_FAR_SHADOW,
                currentFrame,
                i
            );
        }
    }

    m_scene.updateUBO(
        m_camera,
        m_shadowMap->getLightSpace(),
        m_swapchain->getExtent(),
        currentFrame
    );

    //--------------------Acquires an image from the swapchain------------------

    const uint32_t imageIndex = m_swapchain->getNextImageIndex(m_imageAvailableSemaphores[currentFrame]);


    //---------------------Records all the command buffer-----------------------    

    // Shadow Mapping
    const VkExtent2D shadowExtent = { 2 * m_swapchain->getExtent().height, 2 * m_swapchain->getExtent().width };

    recordCommandBuffer(
        m_shadowMap->getFramebuffer(imageIndex),
        m_shadowMap->getRenderPass(),
        shadowExtent,
        { &m_shadowMap->getGraphicsPipeline() },
        currentFrame,
        m_shadowMap->getCommandBuffer(currentFrame),
        m_clearValuesShadowMap,
        m_shadowMap->getCommandPool()
    );

    // Scene
    recordCommandBuffer(
        m_swapchain->getFramebuffer(imageIndex),
        m_scene.getRenderPass(),
        m_swapchain->getExtent(),
        { &m_scene.getLightPipeline(),&m_scene.getPBRpipeline(), &m_scene.getSkyboxPipeline() },
        currentFrame,
        m_commandPoolForGraphics->getCommandBuffer(currentFrame),
        m_clearValues,
        m_commandPoolForGraphics
    );

    // GUI
    m_GUI->recordCommandBuffer(currentFrame, imageIndex, m_clearValues);

    //----------------------Submits the command buffer -------------------------

    std::vector<VkCommandBuffer> commandBuffersToSubmit = { m_shadowMap->getCommandBuffer(currentFrame), m_commandPoolForGraphics->getCommandBuffer(currentFrame),m_GUI->getCommandBuffer(currentFrame) };

    std::vector<VkSemaphore> waitSemaphores = { m_imageAvailableSemaphores[currentFrame] };
    std::vector<VkSemaphore> signalSemaphores = { m_renderFinishedSemaphores[currentFrame] };

    m_commandPoolForGraphics->submitCommandBuffer(
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

void Renderer::calculateFrames(double& lastTime, int& framesCounter)
{
    double currentTime = glfwGetTime();
    framesCounter++;

    if (currentTime - lastTime >= 1.0)
    {
        m_mpf = 1000.0 / double(framesCounter);

        framesCounter = 0;
        lastTime += 1.0;
    }
}


void Renderer::mainLoop()
{
    // Tells us in which frame we are,
    // between 1 <= frame <= MAX_FRAMES_IN_FLIGHT

    uint8_t currentFrame = 0;

    double lastTime = glfwGetTime();
    int framesCounter = 0;

    while (m_window->isWindowClosed() == false)
    {
        calculateFrames(lastTime, framesCounter);

        handleInput();
        m_GUI->draw(
            m_scene.getModels(),
            m_camera,
            m_scene.getObjectModelIndices(),
            m_scene.getLightModelIndices(),
            m_device->getDeviceName(),
            m_mpf,
            m_msaa.getSamplesCount(),
            m_device->getApiVersion()
        );
        drawFrame(currentFrame);
    }
    vkDeviceWaitIdle(m_device->getLogicalDevice());
}

void Renderer::doComputations()
{
    std::vector<Computation> computations = { m_scene.getComputation() };

    std::cout << "Doing computations.\n";

    const VkCommandBuffer& commandBuffer = (m_commandPoolForCompute->getCommandBuffer(0));

    for (auto& computation : computations)
    {
        // Resets the command buffer to be able to be recorded.
        m_commandPoolForCompute->resetCommandBuffer(0);
        // Specifies some details about the usage of this specific command buffer.
        m_commandPoolForCompute->beginCommandBuffer(0, commandBuffer);

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


        vkCmdPipelineBarrier(
            commandBuffer,
            // Pipeline stage in which operations occur that should happen before the barrier.
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            // Pipeline stage in which operations will wait on the barrier.
            VK_PIPELINE_STAGE_HOST_BIT,
            // 0 or VK_DEPENDENCY_BY_REGION_BIT(per-region condition)
            0,
            // References arrays of pipeline barries of the three available
            // types: memory barriers, buffer memory barriers, and image memory
            // barriers.
            1,  &readBarrier,
            0, {},
            0, {}
        );

        m_commandPoolForCompute->endCommandBuffer(commandBuffer);

        m_commandPoolForCompute->submitCommandBuffer(m_qfHandles.computeQueue, { commandBuffer }, true);
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

    // Scenes
    m_scene.destroy();

    // Models -> Buffers, Memories and Textures.
    m_shadowMap->destroy();
   
    // Descriptor Pool
    m_descriptorPoolForGraphics.destroy();
    m_descriptorPoolForComputations.destroy();

    // Sync objects
    destroySyncObjects();

    // Command Pools
    if (m_commandPoolForGraphics) m_commandPoolForGraphics->destroy();
    if (m_commandPoolForCompute)  m_commandPoolForCompute->destroy();

    //VM Allocator
    vmaDestroyAllocator(m_vmaAllocator);

    // Logical Device
    vkDestroyDevice(m_device->getLogicalDevice(), nullptr);

    // Window Surface
    m_window->destroySurface(m_vkInstance->get());

    // Vulkan's instance
    m_vkInstance->destroy();

    // GLFW
    m_window->destroy();
}

void Renderer::addSkybox(const std::string& fileName, const std::string& textureFolderName)
{
    m_modelsToLoadInfo.push_back({
         ModelType::SKYBOX,
         fileName,
         textureFolderName,fileName,
         glm::fvec3(0.0f),
         glm::fvec3(0.0f),
         glm::fvec3(0.0f),
         glm::fvec3(0.0f),
         LightType::NONE,
         glm::fvec3(0.0f)
        });
}

void Renderer::addObjectPBR(const std::string& name,
    const std::string& folderName, const std::string& fileName,
    const glm::fvec3& pos,
    const glm::fvec3& rot,
    const glm::fvec3& size ) 
{
    m_modelsToLoadInfo.push_back({
         ModelType::NORMAL_PBR,
         name,
         folderName,
         fileName,
         glm::fvec3(0.0f),
         pos,
         rot,
         size,
         LightType::NONE,
         glm::fvec3(0.0f)
        });
}

void Renderer::addDirectionalLight(
    const std::string& name,
    const std::string& folderName,const std::string& fileName,
    const glm::fvec3& color,
    const glm::fvec3& pos,
    const glm::fvec3& endPos,
    const glm::fvec3& size
) {
    m_modelsToLoadInfo.push_back({
        ModelType::LIGHT,
        name,
         folderName,
         fileName,
        color,
        pos,
        glm::fvec3(0.0f),
        size,
        LightType::DIRECTIONAL_LIGHT,
        endPos
        });
}

void Renderer::addPointLight(
    const std::string& name,
    const std::string& folderName,const std::string& fileName,
    const glm::fvec3& color,
    const glm::fvec3& pos,
    const glm::fvec3& size
) {
    m_modelsToLoadInfo.push_back({
          ModelType::LIGHT,
          name,
          folderName,
          fileName,
          color,
          pos,
          glm::fvec3(0.0f),
          size,
          LightType::POINT_LIGHT,
          glm::fvec3(0.0f)
        });
}

void Renderer::addSpotLight(
    const std::string& name,
    const std::string& folderName,
    const std::string& fileName,
    const glm::fvec3& color,
    const glm::fvec3& pos,
    const glm::fvec3& endPos,
    const glm::fvec3& rot,
    const glm::fvec3& size
) {
    m_modelsToLoadInfo.push_back({
          ModelType::LIGHT,
          name,
          folderName,
          fileName,
          color,
          pos,
          rot,
          size,
          LightType::SPOT_LIGHT,
          endPos
        });
}


VkResult Renderer::vhMemCreateVMAAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& allocator)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    return vmaCreateAllocator(&allocatorInfo, &allocator);
}