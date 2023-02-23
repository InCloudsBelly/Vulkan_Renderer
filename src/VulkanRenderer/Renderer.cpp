#include "VulkanRenderer/Renderer.h"

#include <iostream>
#include <vector>
#include <set>
#include <cstring>
#include <limits>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer/Settings/config.h"
#include "VulkanRenderer/Settings/VkLayersConfig.h"
#include "VulkanRenderer/ValidationLayersManager/vlManager.h"
#include "VulkanRenderer/Window/Window.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyHandles.h"
#include "VulkanRenderer/Swapchain/Swapchain.h"
#include "VulkanRenderer/ShaderManager/ShaderManager.h"
#include "VulkanRenderer/GraphicsPipeline/GraphicsPipelineManager.h"
#include "VulkanRenderer/Commands/CommandPool.h"
#include "VulkanRenderer/Extensions/ExtensionsUtils.h"
#include "VulkanRenderer/Buffers/BufferManager.h"
#include "VulkanRenderer/Buffers/BufferUtils.h"
#include "VulkanRenderer/Model/Vertex.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Descriptors/DescriptorTypes.h"
#include "VulkanRenderer/Textures/Texture.h"
#include "VulkanRenderer/DepthBuffer/DepthBuffer.h"


void Renderer::run()
{
    if (m_models.size() == 0)
    {
        std::cout << "Nothing to render..\n";
        return;
    }

    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Renderer::initWindow()
{
    m_window.createWindow(Config::RESOLUTION_W,Config::RESOLUTION_H,Config::TITLE);
}

void Renderer::createSyncObjects()
{
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
        if (vkCreateSemaphore(m_device.getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphore!");
        
        if (vkCreateSemaphore(m_device.getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphore!");
        
        if (vkCreateFence(m_device.getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create fence!");
    }
}

void Renderer::createVkInstance()
{
    if (VkLayersConfig::VALIDATION_LAYERS_ENABLED &&
        !vlManager::AllRequestedLayersAvailable()
        ) {
        throw std::runtime_error(
            "Validation layers requested, but not available!"
        );
    }

    // This data is optional, but it may provide some useful information
    // to the driver in order to optimize our specific application(e.g because
    // it uses a well-known graphics engine with certain special behavior).
    //
    // Example: Our game uses UE and nvidia launched a new driver that optimizes
    // a certain thing. So in that case, nvidia will know it can apply that
    // optimization verifying this info.
    VkApplicationInfo appInfo{};

    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = Config::TITLE;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // This data is not optional and tells the Vulkan driver which global
    // extensions and validation layers we want to use.
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extensions = extensionsUtils::getRequiredExtensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // This variable is placed outside the if statement to ensure that it is
    // not destroyed before the vkCreateInstance call.
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    if (VkLayersConfig::VALIDATION_LAYERS_ENABLED)
    {

        createInfo.enabledLayerCount = static_cast<uint32_t> (
            VkLayersConfig::VALIDATION_LAYERS.size()
            );
        createInfo.ppEnabledLayerNames = VkLayersConfig::VALIDATION_LAYERS.data();

        debugCreateInfo = vlManager::getDebugMessengerCreateInfo();

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

    }
    else
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }


    // -------------------------------------------------------------
    // Paramet. 1 -> Pointer to struct with creation info.
    // Paramet. 2 -> Pointer to custom allocator callbacks.
    // Paramet. 3 -> Pointer to the variable that stores the handle to the
    //               new object.
    // -------------------------------------------------------------
    if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Vulkan's instance!");
}


void Renderer::initVulkan()
{
    createVkInstance();

    vlManager::createDebugMessenger(m_vkInstance, m_debugMessenger);
    m_window.createSurface(m_vkInstance);

    m_device.pickPhysicalDevice(m_vkInstance,m_qfIndices,m_window.getSurface(),m_swapchain);
    m_device.createLogicalDevice(m_qfIndices);

    m_qfHandles.setQueueHandles(m_device.getLogicalDevice(), m_qfIndices);

    m_swapchain.createSwapchain(m_device.getPhysicalDevice(), m_device.getLogicalDevice(), m_window);

    m_swapchain.createAllImageViews(m_device.getLogicalDevice());

    m_renderPass.createRenderPass(m_device.getPhysicalDevice(),m_device.getLogicalDevice(),m_swapchain.getImageFormat());

    m_descriptorPool.createDescriptorSetLayout(m_device.getLogicalDevice());

    m_graphicsPipelineM.createGraphicsPipeline(m_device.getLogicalDevice(),m_swapchain.getExtent(), m_renderPass.getRenderPass(),m_descriptorPool.getDescriptorSetLayout());

    m_depthBuffer.createDepthBuffer(m_device.getPhysicalDevice(),m_device.getLogicalDevice(),m_swapchain.getExtent());

    m_swapchain.createFramebuffers(m_device.getLogicalDevice(), m_renderPass.getRenderPass(), m_depthBuffer);


    // Command Pool #1
    const uint32_t cmdPoolIndex = 0;
    CommandPool newCommandPool(m_device.getLogicalDevice(), m_qfIndices);
    m_commandPools.push_back(newCommandPool);

    for (auto& model : m_models)
    {
        BufferManager::createBufferAndTransferToDevice(
            m_commandPools[cmdPoolIndex],
            m_device.getPhysicalDevice(),
            m_device.getLogicalDevice(),
            model->vertices,
            m_qfHandles.graphicsQueue,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            model->vertexMemory,
            model->vertexBuffer
        );

        // Index Buffer(with staging buffer)
        BufferManager::createBufferAndTransferToDevice(
            m_commandPools[cmdPoolIndex],
            m_device.getPhysicalDevice(),
            m_device.getLogicalDevice(),
            model->indices,
            m_qfHandles.graphicsQueue,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            model->indexMemory,
            model->indexBuffer
        );

        // Create Textures
        model->createTexture(
            m_device.getPhysicalDevice(),
            m_device.getLogicalDevice(),
            VK_FORMAT_R8G8B8A8_SRGB,
            m_commandPools[0],
            m_qfHandles.graphicsQueue
        );
    }

    // Uniform Buffers
    m_descriptorPool.createUniformBuffers(m_device.getPhysicalDevice(), m_device.getLogicalDevice(),Config::MAX_FRAMES_IN_FLIGHT);

    // Descriptor Pool
    m_descriptorPool.createDescriptorPool(m_device.getLogicalDevice(),Config::MAX_FRAMES_IN_FLIGHT,Config::MAX_FRAMES_IN_FLIGHT);

    // Descriptor Sets(of each type of model)
    for (auto& model : m_models)
    {
        m_descriptorPool.createDescriptorSets(
            m_device.getLogicalDevice(),
            model->texture.getTextureImageView(),
            model->texture.getTextureSampler(),
            model->descriptorSets
        );
    }
    // Allocates all the command buffers in the command Pool #1
    m_commandPools[cmdPoolIndex].allocAllCommandBuffers();

    createSyncObjects();
}

void Renderer::drawFrame(uint8_t& currentFrame)
{
    // Waits until the previous frame has finished.
   //    - 2 param. -> FenceCount.
   //    - 4 param. -> waitAll.
   //    - 5 param. -> timeOut.
    vkWaitForFences(m_device.getLogicalDevice(), 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // After waiting, we need to manually reset the fence.
    vkResetFences(m_device.getLogicalDevice(), 1, &m_inFlightFences[currentFrame]);


    //------------------------Updates uniform buffer----------------------------
    m_descriptorPool.updateUniformBuffer1(m_device.getLogicalDevice(),currentFrame,m_swapchain.getExtent());


    //--------------------Acquires an image from the swapchain------------------
    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_device.getLogicalDevice(), m_swapchain.getSwapchain(), UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);


    //---------------------Records all the command buffer-----------------------    
    // Resets the command buffer to be able to be recorded/written.
    m_commandPools[0].resetCommandBuffer(currentFrame);
    m_commandPools[0].recordCommandBuffer(
        m_swapchain.getFramebuffer(imageIndex),
        m_renderPass.getRenderPass(),
        m_swapchain.getExtent(),
        m_graphicsPipelineM.getGraphicsPipeline(),
        currentFrame,
        m_models[0]->vertexBuffer,
        m_models[0]->indexBuffer,
        m_models[0]->indices.size(),
        m_graphicsPipelineM.getPipelineLayout(),
        m_models[0]->descriptorSets
    );


    //----------------------Submits the command buffer -------------------------

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    // Specifies which semaphores to wait on before execution begins.
    submitInfo.pWaitSemaphores = waitSemaphores;
    // Specifies which stage/s of the pipeline to wait.
    submitInfo.pWaitDstStageMask = waitStages;
    // These two commands specify which command buffers to actualy submit for
   // execution.
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(m_commandPools[0].getCommandBuffer(currentFrame));
    // Specifies which semaphores to signal once the command buffer/s have
   // finished execution.
    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if(vkQueueSubmit(m_qfHandles.graphicsQueue, 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");


    //-------------------Presentation of the swapchain image--------------------
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { m_swapchain.getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    vkQueuePresentKHR(m_qfHandles.presentQueue, &presentInfo);

    // Updates the frame
    currentFrame = (currentFrame + 1) % Config::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::mainLoop()
{
    // Tells us in which frame we are,
    // between 1 <= frame <= MAX_FRAMES_IN_FLIGHT
    uint8_t currentFrame = 0;
    while (m_window.isWindowClosed() == false)
    {
        m_window.pollEvents();
        drawFrame(currentFrame);
    }
    vkDeviceWaitIdle(m_device.getLogicalDevice());
}

void Renderer::destroySyncObjects()
{
    for (size_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_device.getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device.getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_device.getLogicalDevice(), m_inFlightFences[i], nullptr);
    }
}

void Renderer::cleanup()
{
    // DepthBuffer
    m_depthBuffer.destroyDepthBuffer(m_device.getLogicalDevice());

    // Framebuffers
    m_swapchain.destroyFramebuffers(m_device.getLogicalDevice());

    // ViewImages of the images from the Swapchain
    m_swapchain.destroyImageViews(m_device.getLogicalDevice());

    // Swapchain
    m_swapchain.destroySwapchain(m_device.getLogicalDevice());

    // Graphics Pipeline
    m_graphicsPipelineM.destroyGraphicsPipeline(m_device.getLogicalDevice());

    // Graphics Pipeline Layout
    m_graphicsPipelineM.destroyPipelineLayout(m_device.getLogicalDevice());

    // Render pass
    m_renderPass.destroyRenderPass(m_device.getLogicalDevice());

    // Uniform Buffer and Memory
    m_descriptorPool.destroyUniformBuffersAndMemories(m_device.getLogicalDevice());

    // Descriptor Pool
    m_descriptorPool.destroyDescriptorPool(m_device.getLogicalDevice());

    // Textures
    for (auto& model : m_models)
        model->texture.destroyTexture(m_device.getLogicalDevice());

    // Descriptor Set Layout
    m_descriptorPool.destroyDescriptorSetLayout(m_device.getLogicalDevice());

    // Bufferss
    for (auto& model : m_models)
    {
        BufferManager::destroyBuffer(m_device.getLogicalDevice(), model->vertexBuffer);
        BufferManager::destroyBuffer(m_device.getLogicalDevice(), model->indexBuffer);

        // Buffer Memories
        BufferManager::freeMemory(m_device.getLogicalDevice(), model->vertexMemory);
        BufferManager::freeMemory(m_device.getLogicalDevice(), model->indexMemory);
    }

    // Sync objects
    destroySyncObjects();

    // Command Pool
    for (auto& commandPool : m_commandPools)
        commandPool.destroyCommandPool();

    // Logical Device
    vkDestroyDevice(m_device.getLogicalDevice(), nullptr);

    // Validation Layers
    if (VkLayersConfig::VALIDATION_LAYERS_ENABLED)
    {
        vlManager::destroyDebugUtilsMessengerEXT(
            m_vkInstance,
            m_debugMessenger,
            nullptr
        );
    }

    // Window Surface
    m_window.destroySurface(m_vkInstance);

    // Vulkan's instance
    vkDestroyInstance(m_vkInstance, nullptr);

    // GLFW
    m_window.destroyWindow();
}


void Renderer::addModel(const std::string& meshFile, const std::string& textureFile) 
{
    std::unique_ptr<Model> newModel = std::make_unique<Model>((std::string(MODEL_DIR) + meshFile).c_str(), textureFile);

    m_models.push_back(std::move(newModel));
}