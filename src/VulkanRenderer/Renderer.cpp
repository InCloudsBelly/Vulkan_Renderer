#include "VulkanRenderer/Renderer.h"

#include <iostream>
#include <vector>
#include <set>
#include <cstring>
#include <limits>
#include <algorithm>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanRenderer/Settings/config.h"
#include "VulkanRenderer/Settings/VkLayersConfig.h"
#include "VulkanRenderer/ValidationLayersManager/vlManager.h"
#include "VulkanRenderer/Window/Window.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyHandles.h"
#include "VulkanRenderer/Swapchain/Swapchain.h"
#include "VulkanRenderer/GraphicsPipeline/GraphicsPipelineManager.h"
#include "VulkanRenderer/Commands/CommandPool.h"
#include "VulkanRenderer/Commands/CommandUtils.h"
#include "VulkanRenderer/Extensions/ExtensionsUtils.h"
#include "VulkanRenderer/Buffers/BufferManager.h"
#include "VulkanRenderer/Buffers/BufferUtils.h"
#include "VulkanRenderer/Model/Vertex.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Descriptors/DescriptorSetLayoutUtils.h"
#include "VulkanRenderer/Descriptors/DescriptorTypes/UBO.h"
#include "VulkanRenderer/Descriptors/DescriptorTypes/DescriptorTypes.h"
#include "VulkanRenderer/Descriptors/DescriptorSets.h"
#include "VulkanRenderer/Textures/Texture.h"
#include "VulkanRenderer/DepthBuffer/DepthBuffer.h"
#include "VulkanRenderer/DepthBuffer/DepthUtils.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"
#include "VulkanRenderer/GUI/GUI.h"


glm::fvec3 cameraPos = glm::fvec3(2.0f, 2.0f, 2.0f);


void Renderer::run()
{
    // Improve this!
    // NUMBER OF VK_ATTACHMENT_LOAD_OP_CLEAR == CLEAR_VALUES
    m_clearValues.resize(2);
    m_clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[1].color = { 1.0f, 0.0f };
    m_cameraPos = glm::fvec3(2.0f);

    if (m_allModels.size() == 0)
    {
        std::cout << "Nothing to render..\n";
        return;
    }

    initWindow();
    initVulkan();

    m_GUI = std::make_unique<GUI>(
        m_device.getPhysicalDevice(),
        m_device.getLogicalDevice(),
        m_vkInstance,
        *(m_swapchain.get()),
        m_qfIndices.graphicsFamily.value(),
        m_qfHandles.graphicsQueue,
        m_window
        );

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
    if (VkLayersConfig::VALIDATION_LAYERS_ENABLED &&!vlManager::AllRequestedLayersAvailable()) {
        throw std::runtime_error("Validation layers requested, but not available!"
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

void Renderer::createRenderPass()
{
    // -Attachments
    // Color Attachment
    VkAttachmentDescription colorAttachment{};
    AttachmentUtils::createAttachmentDescription(
        m_swapchain->getImageFormat(),
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorAttachment
    );

    // Depth Attachment
    VkFormat depthFormat = DepthUtils::findSupportedFormat(
        m_device.getPhysicalDevice(),
        {
         VK_FORMAT_D32_SFLOAT,
         VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT
        },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    VkAttachmentDescription depthAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        depthFormat,
        VK_SAMPLE_COUNT_1_BIT,
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

    // Attachment references

    VkAttachmentReference colorAttachmentRef{};
    AttachmentUtils::createAttachmentReference(
        0,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorAttachmentRef
    );

    VkAttachmentReference depthAttachmentRef{};
    AttachmentUtils::createAttachmentReference(
        1,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        depthAttachmentRef
    );

    // Subpasses

    std::vector<VkAttachmentReference> allAttachments = {
       colorAttachmentRef
    };
    VkSubpassDescription subPassDescript{};
    SubPassUtils::createSubPassDescription(
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        allAttachments,
        &depthAttachmentRef,
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
        (
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
            ),
        0,
        // -Destination parameters.
        0,
        (
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
            ),
        (
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
            ),
        dependency
    );

    m_renderPass = RenderPass(
        m_device.getLogicalDevice(),
        { colorAttachment, depthAttachment },
        { subPassDescript },
        { dependency }
    );
}


void Renderer::initVulkan()
{
    createVkInstance();

    vlManager::createDebugMessenger(m_vkInstance, m_debugMessenger);
    m_window.createSurface(m_vkInstance);

    m_device.pickPhysicalDevice(m_vkInstance,m_qfIndices,m_window.getSurface());
    m_device.createLogicalDevice(m_qfIndices);

    m_qfHandles.setQueueHandles(m_device.getLogicalDevice(), m_qfIndices);

    m_swapchain = std::make_unique<Swapchain>(m_device.getPhysicalDevice(), m_device.getLogicalDevice(), m_window, m_device.getSupportedProperties());

    m_swapchain->createAllImageViews(m_device.getLogicalDevice());

    createRenderPass();

    DescriptorSetLayoutUtils::createDescriptorSetLayout(
        m_device.getLogicalDevice(),
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,  (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}
        },
        m_descriptorSetLayout
    );

    m_graphicsPipelineM.createGraphicsPipeline(m_device.getLogicalDevice(), m_swapchain->getExtent(), m_renderPass.get() , m_descriptorSetLayout);

    m_depthBuffer.createDepthBuffer(m_device.getPhysicalDevice(),m_device.getLogicalDevice(),m_swapchain->getExtent());

    m_swapchain->createFramebuffers(m_device.getLogicalDevice(), m_renderPass.get(), m_depthBuffer);


    // Command Pool #1
    // (improve this)
    m_commandPool = CommandPool(
        m_device.getLogicalDevice(),
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        m_qfIndices.graphicsFamily.value()
    );

    // Allocates all the neccessary cmd buffers in the cmd pool.
    m_commandPool.allocCommandBuffers(Config::MAX_FRAMES_IN_FLIGHT);


    // Descriptor Pool
    m_descriptorPool.createDescriptorPool(
        m_device.getLogicalDevice(),
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,static_cast<uint32_t> (m_allModels.size() * Config::MAX_FRAMES_IN_FLIGHT)},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,static_cast<uint32_t> (m_allModels.size() * Config::MAX_FRAMES_IN_FLIGHT)}
        },
        m_allModels.size() * Config::MAX_FRAMES_IN_FLIGHT
    );

    // Uploads the data from each model to the gpu.
    for (auto& model : m_allModels)
    {
        // Vertex buffer and index buffer(with staging buffer)
        // (Position, color, texCoord, normal, etc)
        model->uploadVertexData(
            m_device.getPhysicalDevice(),
            m_device.getLogicalDevice(),
            m_qfHandles.graphicsQueue,
            m_commandPool
        );

        // Creates and uploads the Texture.
        // (if it hasn't any texture, it will use a default one)
        model->createTexture(
            m_device.getPhysicalDevice(),
            m_device.getLogicalDevice(),
            m_commandPool,
            m_qfHandles.graphicsQueue,
            VK_FORMAT_R8G8B8A8_SRGB
        );

        // Uniform Buffers
        model->createUniformBuffers(
            m_device.getPhysicalDevice(),
            m_device.getLogicalDevice(),
            Config::MAX_FRAMES_IN_FLIGHT
        );

        // Descriptor Sets
        model->createDescriptorSets(
            m_device.getLogicalDevice(),
            m_descriptorSetLayout,
            m_descriptorPool
        );
    }

    createSyncObjects();
}

void Renderer::recordCommandBuffer(
    const VkFramebuffer& framebuffer,
    const RenderPass& renderPass,
    const VkExtent2D& extent,
    const VkPipeline& graphicsPipeline,
    const VkPipelineLayout& pipelineLayout,
    const uint32_t currentFrame,
    const VkCommandBuffer& commandBuffer,
    CommandPool& commandPool
) {
    // Resets the command buffer to be able to be recorded.
    commandPool.resetCommandBuffer(currentFrame);

    // Specifies some details about the usage of this specific command buffer.
    commandPool.beginCommandBuffer(0, commandBuffer);

    //--------------------------------RenderPass-----------------------------

    // The final parameter controls how the drawing commands between the
    // render pass will be provided:
    //    -VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be
    //    embedded in the primary command buffer itself and no secondary
    //    command buffers will be executed.
    //    -VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass
    //    commands will be executed from secondary command buffers.
    renderPass.begin(framebuffer, extent, m_clearValues, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);


    //---------------------------------CMDs-------------------------------
    CommandUtils::STATE::bindPipeline(graphicsPipeline,commandBuffer);
    // Set Dynamic States
    CommandUtils::STATE::setViewport(0.0f, 0.0f, extent, 0.0f, 1.0f, 0, 1, commandBuffer);
    CommandUtils::STATE::setScissor({ 0, 0 }, extent, 0, 1, commandBuffer);

    for (const auto& model : m_allModels)
    {
        CommandUtils::STATE::bindVertexBuffers({ model->getVertexBuffer()}, {0}, 0, 1, commandBuffer);
        CommandUtils::STATE::bindIndexBuffer(model->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32, commandBuffer);
        CommandUtils::STATE::bindDescriptorSets(pipelineLayout, 0, { model->getDescriptorSet(currentFrame) }, {}, commandBuffer);
        CommandUtils::ACTION::drawIndexed(model->getIndexCount(), 1, 0, 0, 0, commandBuffer);
    }
    renderPass.end(commandBuffer);
    commandPool.endCommandBuffer(commandBuffer);
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
    for (auto& model : m_allModels)
    {
        updateUniformBuffer(m_device.getLogicalDevice(), currentFrame, m_swapchain->getExtent(), *model);
    }

    //--------------------Acquires an image from the swapchain------------------
    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_device.getLogicalDevice(), m_swapchain->get(), UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);


    //---------------------Records all the command buffer-----------------------    

    recordCommandBuffer(
        m_swapchain->getFramebuffer(imageIndex),
        m_renderPass,
        m_swapchain->getExtent(),
        m_graphicsPipelineM.getGraphicsPipeline(),
        m_graphicsPipelineM.getPipelineLayout(),
        currentFrame,
        m_commandPool.getCommandBuffer(currentFrame),
        m_commandPool
    );

    // Draws imgui
    m_GUI->recordCommandBuffer(currentFrame, imageIndex, m_clearValues);

    //----------------------Submits the command buffer -------------------------

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    // Specifies which semaphores to wait on before execution begins.
    submitInfo.pWaitSemaphores = waitSemaphores;
    // Specifies which stage/s of the pipeline to wait.
    submitInfo.pWaitDstStageMask = waitStages;
    // These two commands specify which command buffers to actualy submit for execution.
     


    // ImGUI added
    std::array<VkCommandBuffer, 2> submitCommandBuffers = { m_commandPool.getCommandBuffer(currentFrame),m_GUI->getCommandBuffer(currentFrame) };

    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers = submitCommandBuffers.data();
    // Specifies which semaphores to signal once the command buffer/s have finished execution.

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_qfHandles.graphicsQueue, 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");


    //-------------------Presentation of the swapchain image--------------------
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { m_swapchain->get() };
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
        m_GUI->draw(m_allModels, m_cameraPos);
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

    // ImGui
    m_GUI->destroy(m_device.getLogicalDevice());

    // Framebuffers
    m_swapchain->destroyFramebuffers(m_device.getLogicalDevice());

    // ViewImages of the images from the Swapchain
    m_swapchain->destroyImageViews(m_device.getLogicalDevice());

    // Swapchain
    m_swapchain->destroySwapchain(m_device.getLogicalDevice());

    // Graphics Pipeline
    m_graphicsPipelineM.destroyGraphicsPipeline(m_device.getLogicalDevice());

    // Graphics Pipeline Layout
    m_graphicsPipelineM.destroyPipelineLayout(m_device.getLogicalDevice());

    // Render pass
    m_renderPass.destroy(m_device.getLogicalDevice());

    // Models -> Buffers, Memories and Textures.
    for (auto& model : m_allModels)
        model->destroy(m_device.getLogicalDevice());
   
    // Descriptor Pool
    m_descriptorPool.destroyDescriptorPool(m_device.getLogicalDevice());

    // Descriptor Set Layout
    DescriptorSetLayoutUtils::destroyDescriptorSetLayout(
        m_device.getLogicalDevice(),
        m_descriptorSetLayout
    );

    // Sync objects
    destroySyncObjects();

    // Command Pools
    m_commandPool.destroy();

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


void Renderer::addNormalModel(const std::string& name, const std::string& meshFile, const std::string& textureFile)
{
    addModel(name, meshFile, textureFile);
    m_normalModelIndices.push_back(m_allModels.size() - 1);
}

void Renderer::addLightModel(const std::string& name, const std::string& meshFile, const std::string& textureFile)
{
    addModel(name, meshFile, textureFile);
    m_lightModelIndices.push_back(m_allModels.size() - 1);
}

void Renderer::addModel(const std::string& name, const std::string& meshFile, const std::string& textureFile)
{
    m_allModels.push_back(std::make_shared<Model>((std::string(MODEL_DIR) + meshFile).c_str(),textureFile,name));
}


void Renderer::updateUniformBuffer(
    const VkDevice& logicalDevice,
    const uint8_t currentFrame,
    const VkExtent2D extent,
    Model& model ) 
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    DescriptorTypes::UniformBufferObject ubo{};
    
    ubo.model = glm::mat4(1.0);
    ubo.model = glm::translate(ubo.model, model.actualPos);

    ubo.model = glm::scale(ubo.model, model.actualSize);
    ubo.model = glm::rotate(ubo.model, model.actualRot.x, glm::vec3(1.0f, 0.0f, 0.0f));

    ubo.model = glm::rotate(ubo.model, model.actualRot.y, glm::vec3(0.0f, 1.0f, 0.0f));

    ubo.model = glm::rotate(ubo.model, model.actualRot.z, glm::vec3(0.0f, 0.0f, 1.0f));


 
    ubo.view = glm::lookAt(m_cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), (extent.width / (float)extent.height), 0.1f, 10.0f);

    // GLM was designed for OpenGl, where the Y coordinate of the clip coord. is
    // inverted. To compensate for that, we have to flip the sign on the scaling
    // factor of the Y axis.
    ubo.proj[1][1] *= -1;

    ubo.lightsCount = 1;
    ubo.lightPositions = m_allModels[m_lightModelIndices[0]]->actualPos;
    //std::cout << "POS: " << m_models[m_lightModels[0]]->actualPos.x << " " << m_models[m_lightModels[0]]->actualPos.y << " " << m_models[m_lightModels[0]]->actualPos.z << std::endl;
    //for (size_t i = 0; i < ubo.lightsCount; i++)
    //{
    //   ubo.lightPositions = m_models[m_lightModels[i]]->centerPos;
    //   //ubo.lightPosition = m_lightModels[i].centerPos;
    //}


    model.updateUBO(logicalDevice, ubo, currentFrame);
}

