#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanRenderer/SwapChain/Swapchain.h"
#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"

class GUI
{
public:
    GUI(
        const VkInstance& vkInstance,
        const std::shared_ptr<Swapchain>& swapchain,
        const uint32_t& graphicsFamilyIndex,
        const VkQueue& graphicsQueue,
        const std::shared_ptr<Window>& window
    );

    ~GUI();

    void recordCommandBuffer(const uint8_t currentFrame, const uint8_t imageIndex, const std::vector<VkClearValue>& clearValues);

    void draw(
        const std::shared_ptr<Camera>& camera, 
        const std::string& deviceName,
        const double mpf,
        const VkSampleCountFlagBits samplesCount,
        const uint32_t apiVersion
    );

    const VkCommandBuffer& getCommandBuffer(const uint32_t index) const;

    const bool isCursorPositionInGUI() const;

    void destroy();

private:

    void displayLightModels();
    void displayCamera(const std::shared_ptr<Camera>& camera);

    void createModelsWindow(const std::shared_ptr<Camera>& camera);

    void createProfilingWindow(
        const std::string& deviceName,
        const double mpf,
        const VkSampleCountFlagBits samplesCount,
        const uint32_t apiVersion
    );

    void createSlider(const std::string& subMenuName, const std::string& sliceName, const float& maxV, const float& minV, float& value);
    void createTransformationsInfo(glm::vec4& pos,glm::vec3& rot, glm::vec3& size, const std::string& modelName);
    void createTranslationSliders(const std::string& name, const std::string& treeNodeName, glm::fvec4& pos, const float minR, const float maxR);
    void createRotationSliders(const std::string& name,glm::fvec3& pos,const float minR,const float maxR);
    void createSizeSliders(const std::string& name, glm::fvec3& pos, const float minR, const float maxR);

    void createRenderPass();
    void createFrameBuffers();
    void uploadFonts(const VkQueue& graphicsQueue);
    void applyStyle();

    std::vector<VkFramebuffer>      m_framebuffers;

    VkCommandPool                   m_commandPool;
    std::vector<VkCommandBuffer>    m_commandBuffers;

    VkDescriptorPool                m_descriptorPool;
    RenderPass                      m_renderPass;

    // Observer pointers
    const Swapchain*                m_opSwapchain;
};