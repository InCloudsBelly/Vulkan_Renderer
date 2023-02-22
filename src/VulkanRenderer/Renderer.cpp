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
#include "VulkanRenderer/Settings/vkLayersConfig.h"
#include "VulkanRenderer/ValidationLayersManager/vlManager.h"
#include "VulkanRenderer/Window/WindowManager.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyHandles.h"
#include "VulkanRenderer/Swapchain/SwapchainManager.h"
#include "VulkanRenderer/ShaderManager/ShaderManager.h"
#include "VulkanRenderer/GraphicsPipeline/GraphicsPipelineManager.h"
#include "VulkanRenderer/Commands/CommandPool.h"
#include "VulkanRenderer/Extensions/ExtensionsUtils.h"
#include "VulkanRenderer/Buffers/BufferManager.h"
#include "VulkanRenderer/MeshLoader/Vertex.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Descriptors/UniformBufferObject.h"

// Contains the pos and the color.
const std::vector<Vertex> data = {
   {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
   {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
   {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
   {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
   0, 1, 2, 2, 3, 0
};

void Renderer::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Renderer::initWindow()
{
    m_windowM.createWindow(config::RESOLUTION_W,config::RESOLUTION_H,config::TITLE);
}

void Renderer::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(config::MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(config::MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(config::MAX_FRAMES_IN_FLIGHT);


    //---------------------------Sync. Objects Info----------------------------


    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Creates the fence in the signaled state, so that the first call to
    // vkWaitForFences() returns immediately since the fence is already signaled.
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;



    //---------------------Creation of Sync. Objects---------------------------

    for (size_t i = 0; i < config::MAX_FRAMES_IN_FLIGHT; i++)
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
    if (vkLayersConfig::VALIDATION_LAYERS_ENABLED &&
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
    appInfo.pApplicationName = config::TITLE;
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

    if (vkLayersConfig::VALIDATION_LAYERS_ENABLED)
    {

        createInfo.enabledLayerCount = static_cast<uint32_t> (
            vkLayersConfig::VALIDATION_LAYERS.size()
            );
        createInfo.ppEnabledLayerNames = vkLayersConfig::VALIDATION_LAYERS.data();

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
    m_windowM.createSurface(m_vkInstance);

    m_device.pickPhysicalDevice(m_vkInstance,m_qfIndices,m_windowM.getSurface(),m_swapchainM);
    m_device.createLogicalDevice(m_qfIndices);

    m_qfHandles.setQueueHandles(m_device.getLogicalDevice(), m_qfIndices);

    m_swapchainM.createSwapchain(m_device.getPhysicalDevice(), m_device.getLogicalDevice(), m_windowM);

    m_swapchainM.createImageViews(m_device.getLogicalDevice());

    m_renderPassM.createRenderPass(m_device.getLogicalDevice(),m_swapchainM.getImageFormat());

    m_descriptorPool.createDescriptorSetLayout(m_device.getLogicalDevice());

    m_graphicsPipelineM.createGraphicsPipeline(m_device.getLogicalDevice(),m_swapchainM.getExtent(), m_renderPassM.getRenderPass(),m_descriptorPool.getDescriptorSetLayout());

    m_swapchainM.createFramebuffers(m_device.getLogicalDevice(),m_renderPassM.getRenderPass());


    // Command Pool #1
    const uint32_t cmdPoolIndex = 0;
    CommandPool newCommandPool(m_device.getLogicalDevice(), m_qfIndices);
    m_commandPools.push_back(newCommandPool);

    // Vertex Buffer(with staging buffer)

    BufferManager::createBufferAndTransferToDevice(
        m_commandPools[cmdPoolIndex],
        m_device.getPhysicalDevice(),
        m_device.getLogicalDevice(),
        data,
        m_qfHandles.graphicsQueue,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        m_memory1,
        m_vertexBuffer
    );

    // Index Buffer(with staging buffer)
    BufferManager::createBufferAndTransferToDevice(
        m_commandPools[cmdPoolIndex],
        m_device.getPhysicalDevice(),
        m_device.getLogicalDevice(),
        indices,
        m_qfHandles.graphicsQueue,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        m_memory2,
        m_indexBuffer
    );

    // Uniform Buffers
    m_descriptorPool.createUniformBuffers(
        m_device.getPhysicalDevice(),
        m_device.getLogicalDevice(),
        config::MAX_FRAMES_IN_FLIGHT
    );

    // Descriptor Pool
    m_descriptorPool.createDescriptorPool(
        m_device.getLogicalDevice(),
        config::MAX_FRAMES_IN_FLIGHT,
        config::MAX_FRAMES_IN_FLIGHT
    );

    // Descriptor Sets
    m_descriptorPool.createDescriptorSets(m_device.getLogicalDevice());


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

    m_descriptorPool.updateUniformBuffer(m_device.getLogicalDevice(),currentFrame,m_swapchainM.getExtent());


    //--------------------Acquires an image from the swapchain------------------

    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_device.getLogicalDevice(), m_swapchainM.getSwapchain(), UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    //----------------------- Records command buffer ---------------------------
    
    // Resets the command buffer to be able to be recorded/written.
    m_commandPools[0].resetCommandBuffer(currentFrame);
    m_commandPools[0].recordCommandBuffer(
        m_swapchainM.getFramebuffer(imageIndex),
        m_renderPassM.getRenderPass(),
        m_swapchainM.getExtent(),
        m_graphicsPipelineM.getGraphicsPipeline(),
        currentFrame,
        m_vertexBuffer,
        m_indexBuffer,
        indices.size(),
        m_graphicsPipelineM.getPipelineLayout(),
        m_descriptorPool.getDescriptorSets()
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

    VkSwapchainKHR swapchains[] = { m_swapchainM.getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    vkQueuePresentKHR(m_qfHandles.presentQueue, &presentInfo);

    // Updates the frame
    currentFrame = (currentFrame + 1) % config::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::mainLoop()
{
    // Tells us in which frame we are,
    // between 1 <= frame <= MAX_FRAMES_IN_FLIGHT
    uint8_t currentFrame = 0;
    while (m_windowM.isWindowClosed() == false)
    {
        m_windowM.pollEvents();
        drawFrame(currentFrame);
    }
    vkDeviceWaitIdle(m_device.getLogicalDevice());
}

void Renderer::destroySyncObjects()
{
    for (size_t i = 0; i < config::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_device.getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device.getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_device.getLogicalDevice(), m_inFlightFences[i], nullptr);
    }
}

void Renderer::cleanup()
{
    // Framebuffers
    m_swapchainM.destroyFramebuffers(m_device.getLogicalDevice());

    // ViewImages of the images from the Swapchain
    m_swapchainM.destroyImageViews(m_device.getLogicalDevice());

    // Swapchain
    m_swapchainM.destroySwapchain(m_device.getLogicalDevice());

    // Uniform Buffer and Memory
    m_descriptorPool.destroyUniformBuffersAndMemories(
        m_device.getLogicalDevice()
    );

    // Descriptor Pool
    m_descriptorPool.destroyDescriptorPool(m_device.getLogicalDevice());

    // Descriptor Set Layout
    m_descriptorPool.destroyDescriptorSetLayout(m_device.getLogicalDevice());

    // Graphics Pipeline
    m_graphicsPipelineM.destroyGraphicsPipeline(m_device.getLogicalDevice());

    // Graphics Pipeline Layout
    m_graphicsPipelineM.destroyPipelineLayout(m_device.getLogicalDevice());

    // Render pass
    m_renderPassM.destroyRenderPass(m_device.getLogicalDevice());

    // Buffers
    BufferManager::destroyBuffer(m_device.getLogicalDevice(), m_vertexBuffer);
    BufferManager::destroyBuffer(m_device.getLogicalDevice(), m_indexBuffer);

    // Buffer Memories
    BufferManager::freeMemory(m_device.getLogicalDevice(), m_memory1);
    BufferManager::freeMemory(m_device.getLogicalDevice(), m_memory2);

    // Sync objects
    destroySyncObjects();

    // Command Pool
    for (auto& commandPool : m_commandPools)
        commandPool.destroyCommandPool();

    // Logical Device
    vkDestroyDevice(m_device.getLogicalDevice(), nullptr);

    // Validation Layers
    if (vkLayersConfig::VALIDATION_LAYERS_ENABLED)
    {
        vlManager::destroyDebugUtilsMessengerEXT(
            m_vkInstance,
            m_debugMessenger,
            nullptr
        );
    }

    // Window Surface
    m_windowM.destroySurface(m_vkInstance);

    // Vulkan's instance
    vkDestroyInstance(m_vkInstance, nullptr);

    // GLFW
    m_windowM.destroyWindow();
}