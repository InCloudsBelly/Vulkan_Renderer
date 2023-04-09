#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <optional>
#include <VMA/vk_mem_alloc.h>

namespace CommandManager
{
    //--------------------------------------------------------------------------------------------------------------------------------
   //command
    VkResult cmdCreateCommandPool(const VkDevice logicalDevice, const VkCommandPoolCreateFlags flags, const uint32_t graphicsFamilyIndex, VkCommandPool* commandPool);

    VkResult cmdCreateCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count, VkCommandBuffer* pBuffers);

    VkResult cmdBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlagBits usageFlags);

    VkResult cmdBeginCommandBuffer(VkDevice device, VkRenderPass renderPass, uint32_t subpass, VkFramebuffer framebuffer, VkCommandBuffer commandBuffer, VkCommandBufferUsageFlagBits usageFlags);

    VkResult cmdSubmitCommandBuffer(
        const VkQueue&                              queue,
        const std::vector<VkCommandBuffer>&         commandBuffers,
        const bool									waitForCompletition,
        const std::vector<VkSemaphore>&             waitSemaphores = {},
        const std::optional<VkPipelineStageFlags>	waitStages = std::nullopt,
        const std::vector<VkSemaphore>&             signalSemaphores = {},
        const std::optional<VkFence>				fence = std::nullopt
    );

    VkCommandBuffer cmdBeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

    VkResult cmdEndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);

    VkResult cmdEndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence waitFence);

};