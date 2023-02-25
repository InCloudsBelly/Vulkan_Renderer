#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Commands/CommandPool.h"
#include "VulkanRenderer/SwapChain/Swapchain.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"

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

    void draw();

    const VkCommandBuffer& getCommandBuffer(const uint32_t index) const;
    void destroy(const VkDevice& logicalDevice);

private:
    std::vector<VkFramebuffer>  m_framebuffers;
    CommandPool                 m_commandPool;
    DescriptorPool              m_descriptorPool;
    RenderPass                  m_renderPass;

    const Swapchain*            m_opSwapchain;
};