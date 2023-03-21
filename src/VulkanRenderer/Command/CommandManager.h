#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <VMA/vk_mem_alloc.h>

#include "VulkanRenderer/Pipeline/Pipeline.h"

namespace CommandManager
{
    //--------------------------------------------------------------------------------------------------------------------------------
   //command
    VkResult cmdCreateCommandPool(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkCommandPool* commandPool);

    VkResult cmdCreateCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count, VkCommandBuffer* pBuffers);

    VkResult cmdBeginCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkCommandBufferUsageFlagBits usageFlags);

    VkResult cmdBeginCommandBuffer(VkDevice device, VkRenderPass renderPass, uint32_t subpass, VkFramebuffer framebuffer, VkCommandBuffer commandBuffer, VkCommandBufferUsageFlagBits usageFlags);

    VkResult cmdSubmitCommandBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence waitFence);

    VkCommandBuffer cmdBeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

    VkResult cmdEndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);

    VkResult cmdEndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence waitFence);

};