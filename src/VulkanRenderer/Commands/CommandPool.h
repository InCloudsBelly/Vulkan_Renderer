#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"

class CommandPool
{
public:
	CommandPool();
	~CommandPool();
	void createCommandPool(const VkDevice& logicalDevice, QueueFamilyIndices& queueFamilyIndices);

	void destroyCommandPool();

	void createCommandBufferAllocInfo(VkCommandBufferAllocateInfo& allocInfo);
	void allocCommandBuffer(const VkCommandBufferAllocateInfo& allocInfo);

	const VkCommandBuffer& getCommandBuffer(const uint32_t index);
	void resetCommandBuffer(const uint32_t commandBufferIndex);
	void recordCommandBuffer(
		const VkFramebuffer& framebuffer,
		const VkRenderPass& renderPass,
		const VkExtent2D& extent,
		const VkPipeline& graphicsPipeline,
		const uint32_t commandBufferIndex
	);

private:
	VkDevice m_logicalDevice;
	QueueFamilyIndices m_queueFamilyIndices;

	std::vector<VkCommandBuffer> m_commandBuffers;
	VkCommandPool m_commandPool;
};