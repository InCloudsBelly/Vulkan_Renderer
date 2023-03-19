#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <optional>
#include <VMA/vk_mem_alloc.h>

#include "VulkanRenderer/Queue/QueueFamilyIndices.h"

class CommandPool
{
public:
	CommandPool();
	CommandPool(const VkDevice& logicalDevice, const VkCommandPoolCreateFlags& flags, const uint32_t& graphicsFamilyIndex);
	~CommandPool();

	const VkCommandPool& get() const;
	VkCommandPool get();

	void beginCommandBuffer(const VkCommandBufferUsageFlags& flags, const VkCommandBuffer& commandBuffer);
	void beginCommandBuffer(const VkCommandBufferUsageFlags& flags, const uint32_t& cmdBufferIndex);
	void endCommandBuffer(const VkCommandBuffer& commandBuffer);
	void destroy();
	void allocCommandBuffer(VkCommandBuffer& commandBuffer, const bool isOneTimeUsage);
	
	void allocCommandBuffers(const uint32_t& commandBuffersCount);
	
	void submitCommandBuffer(
		const VkQueue&									queue,
		const std::vector<VkCommandBuffer>&				commandBuffers,
		const bool										waitForCompletition,
		const std::vector<VkSemaphore>&					waitSemaphores = {},
		const std::optional<VkPipelineStageFlags>		waitStages = std::nullopt,
		const std::vector<VkSemaphore>&					signalSemaphores = {},
		const std::optional<VkFence>					fence = std::nullopt
	);
	
	
	const VkCommandBuffer& getCommandBuffer(const uint32_t index) const;
	
	void resetCommandBuffer(const uint32_t index);

	void freeCommandBuffer(VkCommandBuffer& commandBuffer);

private:

	void createCommandBufferAllocateInfo(const uint32_t& commandBuffersCount,VkCommandBufferAllocateInfo& allocInfo);

	VkDevice                     m_logicalDevice;

	VkCommandPool                m_commandPool;

	std::vector<VkCommandBuffer> m_commandBuffers;
};