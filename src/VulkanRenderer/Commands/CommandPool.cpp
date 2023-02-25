#include "VulkanRenderer/Commands/CommandPool.h"

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <memory>
#include <vector>

#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"
#include "VulkanRenderer/Commands/CommandUtils.h"

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


void CommandPool::submitCommandBuffer(const VkQueue& graphicsQueue, const VkCommandBuffer& commandBuffer)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Submits and execute the cmd immediately and wait on this transfer to complete.

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	//freeCommandBuffer(commandBuffer);
}


const VkCommandBuffer& CommandPool::getCommandBuffer(const uint32_t index) const
{
	return m_commandBuffers[index];
}


void CommandPool::createRenderPassBeginInfo(const VkRenderPass& renderPass, const VkFramebuffer& framebuffer,
	const VkExtent2D& extent, const std::vector<VkClearValue>& clearValues, VkRenderPassBeginInfo& renderPassInfo)
{
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	// Binds the framebuffer for the swapchain image we want to draw to.
	renderPassInfo.framebuffer = framebuffer;
	// These two param. define the size of the render area. The render area
	// defines where shader loads and stores will take place. The pixels
	// outside this region will have undefined values. It should match
	// the size of the attachments for best performance.
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;
	// These two param. define the clear values to use for
	// VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as load operation for the
	// color attachment.
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
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

