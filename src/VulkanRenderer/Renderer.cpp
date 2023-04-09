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

#include "VulkanRenderer/Command/CommandManager.h"

#include "VulkanRenderer/Model/Attributes.h"

#include "VulkanRenderer/Descriptor/DescriptorTypes.h"
#include "VulkanRenderer/Descriptor/DescriptorManager.h"

#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Texture/Texture.h"

#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"

#include "VulkanRenderer/Camera/Camera.h"

#include "VulkanRenderer/GUI/GUI.h"

#include "VulkanRenderer/Features/ShadowMap.h"

glm::fvec3 cameraPos = glm::fvec3(2.0f, 2.0f, 2.0f);

Renderer* g_RendererSingleton = nullptr;
RenderResource* g_RenderResource = nullptr;
InputManager* g_InputManager = nullptr;

void Renderer::run()
{
#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    g_RendererSingleton = this;
    g_RenderResource = new RenderResource();
    g_InputManager = new InputManager();
   
    m_window = std::make_shared<Window>(Config::RESOLUTION_W, Config::RESOLUTION_H, Config::WINDOW_TITLE);
    g_InputManager->init(m_window->get());

    initVulkan();

    // -------------------------------Main Pass-----------------------------------
    m_scene = std::make_unique<DeferredRenderPass>();
    //m_scene = std::make_unique<ForwardPBRPass>();
    // ---------------------------------------------------------------------------

    doComputations();

    g_RenderResource->m_camera = Camera(glm::fvec3(3.0f, 2.0f, -0.3f), glm::fvec3(0.0f, 0.0f, -1.0f), glm::fvec3(0.0f, 1.0f, 0.0f));

    mainLoop();
    cleanup();
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

    for (uint32_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphore!");
        
        if (vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphore!");
        
        if (vkCreateFence(m_device->getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create fence!");
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

    createVMAAllocator(m_vkInstance->get(), m_device->getPhysicalDevice(), m_device->getLogicalDevice(), m_vmaAllocator);

    m_swapchain = std::make_unique<Swapchain>(m_device->getPhysicalDevice(), m_device->getLogicalDevice(), m_window, m_device->getSupportedProperties());


    //------------------------------Command Pools----------------------------
    // Graphics Command Pool
    {
        CommandManager::cmdCreateCommandPool(getRendererPointer()->getDevice(),VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,getRendererPointer()->getQueueFamilyIndices().graphicsFamily.value(),&m_commandPoolForGraphics);
        
        m_commandBuffersForGraphics.resize( Config::MAX_FRAMES_IN_FLIGHT);
        CommandManager::cmdCreateCommandBuffers(getRendererPointer()->getDevice(), m_commandPoolForGraphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, Config::MAX_FRAMES_IN_FLIGHT, &m_commandBuffersForGraphics[0]);
    }

    // Compute Command Pool
    {
        CommandManager::cmdCreateCommandPool(getRendererPointer()->getDevice(),VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,getRendererPointer()->getQueueFamilyIndices().computeFamily.value(), &m_commandPoolForCompute);

        m_commandBuffersForCompute.resize(1);
        CommandManager::cmdCreateCommandBuffers(getRendererPointer()->getDevice(),m_commandPoolForCompute,VK_COMMAND_BUFFER_LEVEL_PRIMARY,1,&m_commandBuffersForCompute[0]);
    }

    //------------------------------Descriptor Pools----------------------------
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,static_cast<uint32_t>(m_modelsToLoadInfo.size()) * Config::MAX_FRAMES_IN_FLIGHT * 100 },
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_modelsToLoadInfo.size()) * Config::MAX_FRAMES_IN_FLIGHT * 100 },
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2}
    };
    DescriptorManager::createDescriptorPool(poolSizes, &m_descriptorPool);


    // -------------------------------Global Model Resources------------------------------

    g_RenderResource->loadModels(m_modelsToLoadInfo);
    g_RenderResource->uploadModels(m_qfHandles.graphicsQueue, m_commandPoolForGraphics);


    // -------------------------------Main Features------------------------------
    m_msaa = MSAA(m_swapchain->getExtent(), m_swapchain->getImageFormat());

    m_depthBuffer = DepthBuffer(m_swapchain->getExtent(), m_msaa.getSamplesCount());

    
    createSyncObjects();
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

    //--------------------Acquires an image from the swapchain------------------
    const uint32_t imageIndex = m_swapchain->getNextImageIndex(m_imageAvailableSemaphores[currentFrame]);

    {
        //------------------------Updates uniform buffer----------------------------
        m_scene->updateUBO(m_swapchain->getExtent(), currentFrame);

        //---------------------Records all the command buffer-----------------------   
        m_scene->draw(imageIndex, currentFrame);
    }

    //----------------------Submits the command buffer -------------------------
    std::vector<VkCommandBuffer> commandBuffersToSubmit = {m_commandBuffersForGraphics[currentFrame]};

    std::vector<VkSemaphore> waitSemaphores = { m_imageAvailableSemaphores[currentFrame] };
    std::vector<VkSemaphore> signalSemaphores = { m_renderFinishedSemaphores[currentFrame] };

    CommandManager::cmdSubmitCommandBuffer(
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

        g_RenderResource->m_camera.update(m_mpf);

        m_window->pollEvents();
       
        drawFrame(currentFrame);
    }
    vkDeviceWaitIdle(m_device->getLogicalDevice());
}

void Renderer::doComputations()
{
    //std::vector<Computation> computations = { m_scene->getComputation() };

    //std::cout << "Doing computations.\n";

    //const VkCommandBuffer& commandBuffer = (m_commandBuffersForCompute[0]);

    //for (auto& computation : computations)
    //{
    //    // Resets the command buffer to be able to be recorded.
    //    vkResetCommandBuffer(commandBuffer, 0);
    //    // Specifies some details about the usage of this specific command buffer.
    //    CommandManager::cmdBeginCommandBuffer(commandBuffer, (VkCommandBufferUsageFlagBits) 0);


    //    computation.execute(commandBuffer);

    //    // To make sure that the buffer is ready to be accessed.
    //    VkMemoryBarrier readBarrier = {
    //       VK_STRUCTURE_TYPE_MEMORY_BARRIER,
    //       nullptr,
    //       // Specifies that we'll wait for the shader to finish writing to the
    //       // buffer.
    //       VK_ACCESS_SHADER_WRITE_BIT,
    //       VK_ACCESS_HOST_READ_BIT
    //    };


    //    vkCmdPipelineBarrier(
    //        commandBuffer,
    //        // Pipeline stage in which operations occur that should happen before the barrier.
    //        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //        // Pipeline stage in which operations will wait on the barrier.
    //        VK_PIPELINE_STAGE_HOST_BIT,
    //        // 0 or VK_DEPENDENCY_BY_REGION_BIT(per-region condition)
    //        0,
    //        // References arrays of pipeline barries of the three available
    //        // types: memory barriers, buffer memory barriers, and image memory
    //        // barriers.
    //        1,  &readBarrier,
    //        0, {},
    //        0, {}
    //    );

    //    vkEndCommandBuffer(commandBuffer);

    //    CommandManager::cmdSubmitCommandBuffer(
    //        m_qfHandles.computeQueue, 
    //        { commandBuffer }, 
    //        true
    //    );
    //}

    //std::cout << "All the computations have been completed.\n";

}

void Renderer::destroySyncObjects()
{

#ifdef RELEASE_MODE_ON
    ZoneScoped;
#endif

    for (uint32_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; i++)
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
    g_RenderResource->destroy();
    // MSAA
    m_msaa.destroy();

    // DepthBuffer
    m_depthBuffer.destroy();

    // Swapchain
    m_swapchain->destroy();

    // Scenes
    m_scene->destroy();
   
    // Descriptor Pool
    vkDestroyDescriptorPool(getRendererPointer()->getDevice(), m_descriptorPool, nullptr);

    // Sync objects
    destroySyncObjects();

    // Command Pools
    //if (m_commandPoolForGraphics) m_commandPoolForGraphics->destroy();
    //if (m_commandPoolForCompute)  m_commandPoolForCompute->destroy();

    vkDestroyCommandPool(getRendererPointer()->getDevice(), m_commandPoolForGraphics, nullptr);
    vkDestroyCommandPool(getRendererPointer()->getDevice(), m_commandPoolForCompute, nullptr);

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


VkResult Renderer::createVMAAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& allocator)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    return vmaCreateAllocator(&allocatorInfo, &allocator);
}


SwapChainDesc Renderer::getSwapchainInfo()
{
    SwapChainDesc desc;
    desc.image_format = m_swapchain->getImageFormat();
    desc.extent = m_swapchain->getExtent();
    desc.viewport = &m_swapchain->getViewport();
    desc.scissor = &m_swapchain->getScissor();
    desc.imageViews = m_swapchain->getImageViews();
    return desc;
}

DepthImageDesc Renderer::getDepthImageInfo() 
{
    DepthImageDesc desc;
    desc.depth_image_format = m_depthBuffer.getFormat();
    desc.depth_image_view = &m_depthBuffer.getTexture()->getImageView();
    desc.depth_image = &m_depthBuffer.getTexture()->getImage();
    return desc;
}

MSAADesc Renderer::getMSAAInfo()
{
    MSAADesc desc;
    desc.msaa_sampleCount = m_msaa.getSamplesCount();
    desc.msaa_image_view = &m_msaa.getTexture()->getImageView();
    desc.msaa_image = &m_msaa.getTexture()->getImage();
    return desc;
}