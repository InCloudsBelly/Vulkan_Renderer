#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"

class CommandPool
{
public:
	CommandPool();
	CommandPool(const VkDevice& logicalDevice, QueueFamilyIndices& queueFamilyIndices);
	~CommandPool();

	void beginCommandBuffer(const VkCommandBufferUsageFlags& flags,VkCommandBuffer& commandBuffer);
	void endCommandBuffer(VkCommandBuffer& commandBuffer);

	void destroyCommandPool();

	void allocCommandBuffer(VkCommandBuffer& commandBuffer);
	void submitCommandBuffer(VkQueue& graphicsQueue,VkCommandBuffer& commandBuffer);
	void allocAllCommandBuffers();

	VkCommandBuffer& getCommandBuffer(const uint32_t index);
	
	void resetCommandBuffer(const uint32_t index);

	void freeCommandBuffer(VkCommandBuffer& commandBuffer);

	void createRenderPassBeginInfo(const VkRenderPass& renderPass, const VkFramebuffer& framebuffer,
		const VkExtent2D& extent, const std::vector<VkClearValue>& clearValues, VkRenderPassBeginInfo& renderPassInfo);


private:
	//--------------------------------------------------------------------------

	VkCommandPool m_commandPool;
	VkDevice m_logicalDevice;
	QueueFamilyIndices m_queueFamilyIndices;

	// For now it will be custom. We'll just have 2.
	std::vector<VkCommandBuffer> m_commandBuffers;
};