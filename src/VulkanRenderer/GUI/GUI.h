#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanRenderer/Descriptor/DescriptorPool.h"
#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/SwapChain/Swapchain.h"
#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/Model/Model.h"

class GUI
{
public:
    GUI(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const VkInstance& vkInstance,
        const std::shared_ptr<Swapchain>& swapchain,
        const uint32_t& graphicsFamilyIndex,
        const VkQueue& graphicsQueue,
        const std::shared_ptr<Window>& window
    );

    ~GUI();

    void recordCommandBuffer(const uint8_t currentFrame, const uint8_t imageIndex, const std::vector<VkClearValue>& clearValues);

    void draw(const std::vector<std::shared_ptr<Model>>& models, const std::shared_ptr<Camera>& camera, const std::vector<size_t>& normalModelIndices,const std::vector<size_t>& lightModelIndices);

    const VkCommandBuffer& getCommandBuffer(const uint32_t index) const;

    const bool isCursorPositionInGUI() const;

    void destroy();

private:

    void createLightsWindow(std::vector<std::shared_ptr<Model>> models, const std::vector<size_t> indices);
    void createModelsWindow(std::vector<std::shared_ptr<Model>> models, const std::vector<size_t> indices);
    
    void createSlider(const std::string& subMenuName, const std::string& sliceName, const float& maxV, const float& minV, float& value);
    void createCameraWindow(const std::shared_ptr<Camera>& camera);
    void createTransformationsInfo(glm::vec4& pos,glm::vec3& rot, glm::vec3& size, const std::string& modelName);
    void createTranslationSliders(const std::string& name, const std::string& treeNodeName, glm::fvec4& pos, const float minR, const float maxR);
    void createRotationSliders(const std::string& name,glm::fvec3& pos,const float minR,const float maxR);
    void createSizeSliders(const std::string& name, glm::fvec3& pos, const float minR, const float maxR);

    void createRenderPass();
    void createFrameBuffers();
    void uploadFonts(const VkQueue& graphicsQueue);
    void applyStyle();

    VkDevice                        m_logicalDevice;

    std::vector<VkFramebuffer>      m_framebuffers;
    std::shared_ptr<CommandPool>    m_commandPool;
    DescriptorPool                  m_descriptorPool;
    RenderPass                      m_renderPass;

    // Observer pointers
    const Swapchain*                m_opSwapchain;
};