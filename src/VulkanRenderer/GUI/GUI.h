#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Commands/CommandPool.h"
#include "VulkanRenderer/SwapChain/Swapchain.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/Model/Model.h"

class GUI
{
public:
    GUI(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const VkInstance& vkInstance,
        const Swapchain& swapchain,
        const uint32_t& graphicsFamilyIndex,
        const VkQueue& graphicsQueue,
        Window& window
    );

    ~GUI();

    void recordCommandBuffer(const uint8_t currentFrame, const uint8_t imageIndex, const std::vector<VkClearValue>& clearValues);

    void draw(const std::vector<std::shared_ptr<Model>>& models,glm::fvec4& cameraPos,const std::vector<size_t> normalModelIndices,const std::vector<size_t> lightModelIndices);

    const VkCommandBuffer& getCommandBuffer(const uint32_t index) const;

    const bool isCursorPositionInGUI() const;

    void destroy(const VkDevice& logicalDevice);

private:

    void createLightsWindow(std::vector<std::shared_ptr<Model>> models, const std::vector<size_t> indices);
    void createModelsWindow(std::vector<std::shared_ptr<Model>> models, const std::vector<size_t> indices);
    void createCameraWindow(const std::string& name, glm::fvec4& cameraPos);
    void createTransformationsInfo(glm::vec4& pos,glm::vec3& rot, glm::vec3& size, const std::string& modelName);
    void createTranslationSliders(const std::string& name, glm::fvec4& pos, const float minR, const float maxR);
    void createRotationSliders(const std::string& name,glm::fvec3& pos,const float minR,const float maxR);
    void createSizeSliders(const std::string& name, glm::fvec3& pos, const float minR, const float maxR);

    void createRenderPass();
    void createFrameBuffers();
    void uploadFonts(const VkQueue& graphicsQueue);
    void applyStyle();

    std::vector<VkFramebuffer>  m_framebuffers;
    CommandPool                 m_commandPool;
    DescriptorPool              m_descriptorPool;
    RenderPass                  m_renderPass;

    // Observer pointers
    const Swapchain*            m_opSwapchain;
    const VkDevice*             m_opLogicalDevice;
};