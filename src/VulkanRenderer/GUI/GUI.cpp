#include "VulkanRenderer/GUI/GUI.h"

#include <iostream>
#include <string>
#include <cstring>

#include <imgui.h>
#include <imgui_internal.h>
#include <imstb_rectpack.h>
#include <imstb_textedit.h>
#include <imstb_truetype.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/SwapChain/Swapchain.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/RenderResource.h"

GUI::GUI()
{
    const VkInstance& vkInstance = getRendererPointer()->getVKinstance();
    const VkQueue& graphicsQueue = getRendererPointer()->getGraphicsQueue();
    const uint32_t& graphicsFamilyIndex = getRendererPointer()->getQueueFamilyIndices().graphicsFamily.value();
    const std::shared_ptr<Swapchain> swapchain = getRendererPointer()->getSwapchain();

    // -Descriptor Pool

    // (calculates the total size of the pool depending of the descriptors
    // we send as parameter and the number of descriptor SETS defined)
    std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    DescriptorManager::createDescriptorPool(poolSizes, &m_descriptorPool);
  
    
    // Clear Color
    m_clearValues.resize(2);
    m_clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[1].color = { 1.0f, 0.0f };
    
    // - RenderPass
    createRenderPass();

    // - Imgui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    //ImGui::StyleColorsDark();
    applyStyle();

    ImGui_ImplGlfw_InitForVulkan(getRendererPointer()->getWindow()->get(), true);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = vkInstance;
    initInfo.PhysicalDevice = getRendererPointer()->getPhysicalDevice();
    initInfo.Device = getRendererPointer()->getDevice();
    initInfo.QueueFamily = graphicsFamilyIndex;
    initInfo.Queue = graphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_descriptorPool;
    initInfo.Allocator = nullptr;
    initInfo.MinImageCount = swapchain->getMinImageCount();
    initInfo.ImageCount = swapchain->getImageCount();
    initInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, m_renderPass.get());


    // -Creation of command buffers and command pool
    CommandManager::cmdCreateCommandPool(
        getRendererPointer()->getDevice(),
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        graphicsFamilyIndex,
        &m_commandPool
    );

    m_commandBuffers.resize(Config::MAX_FRAMES_IN_FLIGHT);
    CommandManager::cmdCreateCommandBuffers(
        getRendererPointer()->getDevice(),
        m_commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        Config::MAX_FRAMES_IN_FLIGHT,
        &m_commandBuffers[0]
    );


    uploadFonts(graphicsQueue);

    createFrameBuffers();
}

const bool GUI::isCursorPositionInGUI() const
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse)
        return true;

    return false;
}


void GUI::applyStyle()
{
    auto& style = ImGui::GetStyle();
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.GrabRounding = 4.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.73f, 0.75f, 0.74f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.09f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.84f, 0.66f, 0.66f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.84f, 0.66f, 0.66f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.47f, 0.22f, 0.22f, 0.67f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.47f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.47f, 0.22f, 0.22f, 0.67f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.34f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.71f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.84f, 0.66f, 0.66f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.47f, 0.22f, 0.22f, 0.65f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.71f, 0.39f, 0.39f, 0.65f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_Header] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.84f, 0.66f, 0.66f, 0.65f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.84f, 0.66f, 0.66f, 0.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_Tab] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_TabActive] = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
    
/*
 * -Uploads the fonts to the GPU.
 */
void GUI::uploadFonts(const VkQueue& graphicsQueue)
{
    // (one time command buffer)
    VkCommandBuffer newCommandBuffer = CommandManager::cmdBeginSingleTimeCommands(getRendererPointer()->getDevice(), m_commandPool);
    ImGui_ImplVulkan_CreateFontsTexture(newCommandBuffer);

    CommandManager::cmdEndSingleTimeCommands(getRendererPointer()->getDevice(), graphicsQueue, m_commandPool, newCommandBuffer);

}

void GUI::createFrameBuffers()
{
    m_framebuffers.resize(getRendererPointer()->getSwapchain()->getImageCount());

    VkImageView attachment[1];
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = m_renderPass.get();
    info.attachmentCount = 1;
    info.pAttachments = attachment;
    info.width = getRendererPointer()->getSwapchain()->getExtent().width;
    info.height = getRendererPointer()->getSwapchain()->getExtent().height;
    // The layers is 1 because our imageViews are single images and not
    // arrays.
    info.layers = 1;
    for (uint32_t i = 0; i < getRendererPointer()->getSwapchain()->getImageCount(); i++)
    {
        attachment[0] = getRendererPointer()->getSwapchain()->getImageView(i);
        vkCreateFramebuffer(getRendererPointer()->getDevice(), &info, nullptr, &m_framebuffers[i]);
    }
}

void GUI::createRenderPass()
{
    // - Attachments
    VkAttachmentDescription attachment = {};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        getRendererPointer()->getSwapchain()->getImageFormat(),
        VK_SAMPLE_COUNT_1_BIT,
        // Tells Vulkan to not clear the content of the framebuffer but to
        // draw over it instead.
        // (because we want the GUI to be drawn over our main rendering)
        VK_ATTACHMENT_LOAD_OP_LOAD,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        // We want optimal performance because we are going to draw some
        // stuff.
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // Specifies that this render pass is the last one(because we want
        // to draw it over all the other render passes).
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        attachment
    );

    VkAttachmentReference colorAttachment = {};
    AttachmentUtils::createAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, colorAttachment);


    // - Subpass
    VkSubpassDescription subpass = {};
    SubPassUtils::createSubPassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, &colorAttachment, nullptr, nullptr, subpass);

    // -Synch. between this render pass and the one from the renderer.
    VkSubpassDependency dependency = {};
    SubPassUtils::createSubPassDependency(
        // - Source parameters.
        // To create a dependency outside the current render pass.
        VK_SUBPASS_EXTERNAL,
        // Before drawing the GUI, we want our geometry to be already
        // renderer. That means we want the pixels to be already written to
        // the framebuffer(that's why we use 
        // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT).
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        // 0 means "nothing". Implicit sync. means Vulkan does it for us.
        0,
        // - Destination parameters.
        // Refers to our first and only subpass by its index.
        0,
        // Here we state when we want to draw(also the same thing that we
        // are waiting in srcStageMask).
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        (VkDependencyFlagBits)0,
        dependency
    );

    m_renderPass = RenderPass({ attachment }, { subpass }, { dependency });
}



void GUI::recordCommandBuffer(const uint8_t currentFrame,const uint8_t imageIndex) 
{
    const VkCommandBuffer& commandBuffer = getRendererPointer()->getGraphicsCommandBuffer(currentFrame);

   /* vkResetCommandBuffer(commandBuffer, 0);
    CommandManager::cmdBeginCommandBuffer(commandBuffer, (VkCommandBufferUsageFlagBits)VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);*/

        m_renderPass.begin(m_framebuffers[imageIndex], getRendererPointer()->getSwapchain()->getExtent(), { m_clearValues[currentFrame] }, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        m_renderPass.end(commandBuffer);

  /*  vkEndCommandBuffer(commandBuffer);*/
}

const VkCommandBuffer& GUI::getCommandBuffer(const uint32_t index) const
{
    return m_commandBuffers[index];
}


void GUI::draw(const uint8_t currentFrame, const uint8_t imageIndex)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    float sizeX, sizeY, paddingY;
    {
        sizeX = float(ImGui::GetIO().DisplaySize.x) * 0.2f;
        sizeY = float(ImGui::GetIO().DisplaySize.y) * 0.2f;
        paddingY = 0.05f;
        ImGui::SetNextWindowSize(ImVec2(sizeX, sizeY),ImGuiCond_Always );
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x,0.0f),ImGuiCond_Always,ImVec2(1.0f, 0.0f));
        createProfilingWindow();
    }
    {
        sizeX = float(ImGui::GetIO().DisplaySize.x) * 0.2f;
        sizeY = float(ImGui::GetIO().DisplaySize.y);
        paddingY = 0.05f;
        ImGui::SetNextWindowSize(ImVec2(sizeX, sizeY),ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(sizeX,paddingY),ImGuiCond_Always,ImVec2(1.0f, 0.0f));

        createModelsWindow();
    }
    ImGui::Render();


    recordCommandBuffer(currentFrame, imageIndex);
}


void GUI::createProfilingWindow()
{
    const std::string& deviceName = getRendererPointer()->getDeviceName();
    const double mpf = getRendererPointer()->getMicroSecondPerFrame();
    const VkSampleCountFlagBits& samplesCount = getRendererPointer()->getMSAA()->getSamplesCount();
    const uint32_t& apiVersion = getRendererPointer()->getApiVersion();

    ImGui::Begin("Profiling", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImGui::Columns(2);

    ImGui::Text("GPU: ");
    ImGui::NextColumn();
    ImGui::Text(deviceName.c_str());
    ImGui::NextColumn();
    ImGui::Separator();

    ImGui::Text("Vulkan: ");
    ImGui::NextColumn();
    ImGui::Text(
        (   std::to_string(VK_VERSION_MAJOR(apiVersion)) + "." +
            std::to_string(VK_VERSION_MINOR(apiVersion)) + "." +
            std::to_string(VK_VERSION_PATCH(apiVersion))
        ).c_str()
    );
    ImGui::NextColumn();
    ImGui::Separator();

    ImGui::Text(("ms/frame: "));
    ImGui::NextColumn();
    ImGui::Text(std::to_string(mpf).c_str());
    ImGui::NextColumn();
    ImGui::Separator();

    ImGui::Text(("MSAA: "));
    ImGui::NextColumn();
    ImGui::Text(std::string(std::to_string(samplesCount) + "x").c_str());
    ImGui::NextColumn();
    ImGui::Separator();

    ImGui::End();
}


void GUI::displayLightModels() 
{
    for (auto& info : getRenderResource()->m_lightsInfo)
    {
        const std::string modelName = info.name;
        std::string subMenuName;
        std::string sliderName;

        glm::fvec3 newPos = info.pos;
        glm::fvec3 newRot = info.rot;
        glm::fvec3 newSize = info.size;
        glm::fvec3 color = info.m_color;
        float intensity = info.m_intensity;

        if (ImGui::TreeNode(modelName.c_str()))
        {
            ImGui::ColorEdit3(("Color###" + modelName).c_str(), &(color.x));

            createTransformationsInfo(newPos, newRot, newSize, modelName);

            if (info.m_lightType != LightType::POINT_LIGHT)
            {
                glm::fvec3 targetPos = info.m_targetPos;
                // Target's position.
                createTranslationSliders(modelName, "Target Pos.", targetPos, -100.0f, 100.0f);

                info.m_targetPos =targetPos;
            }

            // Intensity
            subMenuName = ("Intensity###LightProperty::Intensity" + modelName);
            sliderName = ("###Intensity::" + modelName);
            createSlider(subMenuName, sliderName, 100.0f, 0.0f, intensity);

            ImGui::TreePop();
            ImGui::Separator();
        }
        info.pos = newPos;
        info.modelPtr->setPos(newPos);
        info.rot = (newRot);
        info.size = (newSize);
        info.m_intensity = (intensity);
        info.m_color = (color);
    }
}


void GUI::createSlider(const std::string& subMenuName,const std::string& sliceName,const float& maxV,const float& minV,float& value) 
{
    if (ImGui::TreeNode(subMenuName.c_str())) 
    {
        ImGui::SliderFloat(sliceName.c_str(),&value,minV, maxV);

        ImGui::TreePop();
        ImGui::Separator();
    }
}


void GUI::createModelsWindow()
{
    ImGui::Begin("Models",NULL,ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::Text("Objects");
    ImGui::Separator();

    for (auto ptr : getRenderResource()->m_normalModels)
    {
        const std::string modelName = ptr->getName();

        glm::fvec3 newPos = ptr->getPos();
        glm::fvec3 newRot = ptr->getRot();
        glm::fvec3 newSize = ptr->getSize();
        bool isHidden = ptr->isHidden();

        if (ImGui::TreeNode(modelName.c_str()))
        {
            ImGui::Checkbox("Hide", &isHidden);

            createTransformationsInfo(newPos, newRot, newSize, modelName);

            ImGui::TreePop();
            ImGui::Separator();
        }
        ptr->setPos(newPos);
        ptr->setRot(newRot);
        ptr->setSize(newSize);
        ptr->setHideStatus(isHidden);
    }
   

    ImGui::Text("Lights");
    ImGui::Separator();
    displayLightModels();

    ImGui::Text("Camera");
    ImGui::Separator();
    displayCamera();

    ImGui::End();
}

void GUI::createTranslationSliders(const std::string& name, const std::string& treeNodeName, glm::fvec3& pos,const float minR,const float maxR)
{
    const std::vector<std::string> sliderNames = { "X##TRANSLATION::" + name,"Y##TRANSLATION::" + name,"Z##TRANSLATION::" + name };

    std::vector<float*> values = { &pos.x,&pos.y,&pos.z };

    if (ImGui::TreeNode((treeNodeName + "##TRANSLATION::" + name).c_str()))
    {
        for (uint32_t i = 0; i < sliderNames.size(); i++)
        {
            ImGui::SliderFloat(sliderNames[i].c_str(), values[i], minR, maxR);
        }
        ImGui::TreePop();
        ImGui::Separator();
    }
}

void GUI::createRotationSliders( const std::string& name,glm::fvec3& pos,const float minR,const float maxR) 
{
    const std::vector<std::string> sliderNames = { "X##ROTATION::" + name, "Y##ROTATION::" + name,"Z##ROTATION::" + name };

    std::vector<float*> values = { &pos.x,&pos.y,&pos.z };

    if (ImGui::TreeNode(("Rotation##ROTATION::" + name).c_str())) 
    {
        for (uint32_t i = 0; i < sliderNames.size(); i++)
        {
            ImGui::SliderFloat(sliderNames[i].c_str(),values[i],minR,maxR);
        }
        ImGui::TreePop();
        ImGui::Separator();
    }
}

void GUI::createSizeSliders(const std::string& name,glm::fvec3& pos,const float minR,const float maxR) 
{
    const std::vector<std::string> sliderNames = { "X##SIZE::" + name,"Y##SIZE::" + name,"Z##SIZE::" + name };
    std::vector<float*> values = { &pos.x,&pos.y,&pos.z };

    if (ImGui::TreeNode(("Scale##SIZE::" + name).c_str())) {
        for (uint32_t i = 0; i < sliderNames.size(); i++)
        {
            ImGui::SliderFloat(sliderNames[i].c_str(),values[i],minR,maxR);
        }
        ImGui::TreePop();
        ImGui::Separator();
    }
}

void GUI::createTransformationsInfo(glm::vec3& pos,glm::vec3& rot,glm::vec3& size, const std::string& modelName)
{
    createTranslationSliders(modelName, "Position", pos, -100.0f, 100.0f);
    createRotationSliders(modelName, rot, -5.0f, 5.0f);
    createSizeSliders(modelName, size, 0.0f, 1.0f);
}

void GUI::displayCamera()
{
    glm::fvec3 cameraPos = getRenderResource()->m_camera.getCameraPos();
    glm::fvec3 lookAtDir = getRenderResource()->m_camera.getCameraFront();

    // TODO: Name based on the camera type.
    if (ImGui::TreeNode("Arcball"))
    {
        // Moves the camera.
        createTranslationSliders("Camera", "Position", cameraPos, -100.0f, 100.0f);
        // Moves the target's position.
        createTranslationSliders("Camera", "FrontDir.", lookAtDir, -100.0f, 100.0f);
        ImGui::TreePop();
    }
}

void GUI::destroy()
{
    for (auto& framebuffer : m_framebuffers)
        vkDestroyFramebuffer(getRendererPointer()->getDevice(), framebuffer, nullptr);

    m_renderPass.destroy();
    vkDestroyCommandPool(getRendererPointer()->getDevice(), m_commandPool, nullptr);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(getRendererPointer()->getDevice(), m_descriptorPool, nullptr);
}

GUI::~GUI() {}