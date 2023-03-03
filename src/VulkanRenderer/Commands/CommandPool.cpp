#include "VulkanRenderer/Commands/CommandPool.h"

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <memory>
#include <vector>

#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Queue/QueueFamilyIndices.h"
#include "VulkanRenderer/Commands/CommandManager.h"

CommandPool::CommandPool() {}

CommandPool::~CommandPool() {}

CommandPool::CommandPool(const VkDevice& logicalDevice, const VkCommandPoolCreateFlags& flags, const uint32_t& graphicsFamilyIndex)
	: m_logicalDevice(logicalDevice) 
{

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = flags;
	poolInfo.queueFamilyIndex = graphicsFamilyIndex;

	if (vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool!");
}

const VkCommandPool& CommandPool::get() const
{
	return m_commandPool;
}


void CommandPool::destroy()
{
	vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
}

void CommandPool::createCommandBufferAllocateInfo(const uint32_t& commandBuffersCount,VkCommandBufferAllocateInfo& allocInfo) 
{
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = commandBuffersCount;
}

void CommandPool::allocCommandBuffers(const uint32_t& commandBuffersCount)
{
	const uint32_t oldSize = m_commandBuffers.size();
	m_commandBuffers.resize(oldSize + commandBuffersCount);

	VkCommandBufferAllocateInfo allocInfo{};
	createCommandBufferAllocateInfo(commandBuffersCount, allocInfo);

	vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &m_commandBuffers[oldSize]);

}

void CommandPool::allocCommandBuffer(VkCommandBuffer& commandBuffer, const bool isOneTimeUsage)
{
	VkCommandBufferAllocateInfo allocInfo{};
	createCommandBufferAllocateInfo(1, allocInfo);

	vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &commandBuffer);

	if (isOneTimeUsage == false)
		m_commandBuffers.push_back(commandBuffer);
}


void CommandPool::submitCommandBuffer(
	const VkQueue&									queue,
	const std::vector<VkCommandBuffer>&				commandBuffers,
	const bool										waitForCompletition,
	const std::optional<std::vector<VkSemaphore>>	waitSemaphores,
	const std::optional<VkPipelineStageFlags>		waitStages,
	const std::optional<std::vector<VkSemaphore>>	signalSemaphores,
	const std::optional<VkFence>					fence
)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = commandBuffers.size();
	submitInfo.pCommandBuffers = commandBuffers.data();

	if (waitSemaphores.has_value())
	{
		submitInfo.waitSemaphoreCount = waitSemaphores.value().size();
		// Specifies which semaphores to wait on before execution begins.
		submitInfo.pWaitSemaphores = waitSemaphores.value().data();
	}

	if (waitStages.has_value())
	{
		// Specifies which stage/s of the pipeline to wait.
		submitInfo.pWaitDstStageMask = &(waitStages.value());
	}

	if (signalSemaphores.has_value())
	{
		// Specifies which semaphores to signal once the command buffer/s have
		// finished execution.
		submitInfo.signalSemaphoreCount = signalSemaphores.value().size();
		submitInfo.pSignalSemaphores = signalSemaphores.value().data();
	}

	// Submits and execute the cmd immediately and wait on this transfer to complete.

	auto status = vkQueueSubmit(queue, 1, &submitInfo, (fence.has_value()) ? fence.value() : VK_NULL_HANDLE);
	if (status != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer!");

	if (waitForCompletition)
		vkQueueWaitIdle(queue);
}


const VkCommandBuffer& CommandPool::getCommandBuffer(const uint32_t index) const
{
	return m_commandBuffers[index];
}


void CommandPool::beginCommandBuffer(const VkCommandBufferUsageFlags& flags,const uint32_t& cmdBufferIndex)
{
	beginCommandBuffer(flags, m_commandBuffers[cmdBufferIndex]);
}


void CommandPool::beginCommandBuffer(const VkCommandBufferUsageFlags& flags,const VkCommandBuffer& commandBuffer)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// Optional
	// Specifies how we're goint to use the command buffer:
	//    -VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer
	//    will be rerecorded right after executing it once.
	//    -VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary
	//    command buffer that will be entirely within a single render pass.
	//    -VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer
	//    can be resubmitted while it is also already pending execution.
	// For now we won't use any of them.
	beginInfo.flags = flags;
	// Optional
	// Relevant only for secondary command buffers. It specifies which state
	// to inherit from the calling primary command buffers.
	beginInfo.pInheritanceInfo = nullptr;

	// If the command buffer was already recorded/writed once, then a call
	// to vkBeginCommandBuffer will implicity reset it. It's not possible
	// to append commands to a buffer at a later time.
	auto status = vkBeginCommandBuffer(commandBuffer,&beginInfo);

	if (status != VK_SUCCESS)
		throw std::runtime_error("Failed to begin recording command buffer!");
}

void CommandPool::endCommandBuffer(const VkCommandBuffer& commandBuffer)
{
	auto status = vkEndCommandBuffer(commandBuffer);
	if (status != VK_SUCCESS)
		throw std::runtime_error("Failed to record command buffer!");
}

void CommandPool::resetCommandBuffer(const uint32_t index)
{
	vkResetCommandBuffer(m_commandBuffers[index], 0);
}

void CommandPool::freeCommandBuffer(VkCommandBuffer& commandBuffer)
{
	vkFreeCommandBuffers(m_logicalDevice, m_commandPool, 1, &commandBuffer);
}

