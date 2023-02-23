#include "VulkanRenderer/Commands/CommandPool.h"

#include <vulkan/vulkan.h>
#include <stdexcept>

#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"
#include "VulkanRenderer/Commands/CommandUtils.h"

CommandPool::CommandPool() {}

CommandPool::CommandPool(const VkDevice& logicalDevice, QueueFamilyIndices& queueFamilyIndices)
{
	m_logicalDevice = logicalDevice;
	m_queueFamilyIndices = queueFamilyIndices;
	m_commandBuffers.resize(Config::MAX_FRAMES_IN_FLIGHT);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool!");
}

void CommandPool::destroyCommandPool()
{
	vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
}

void CommandPool::allocCommandBuffer(VkCommandBuffer& commandBuffer)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &commandBuffer);
}

void CommandPool::submitCommandBuffer(VkQueue& graphicsQueue, VkCommandBuffer& commandBuffer)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Submits and execute the cmd immediately and wait on this transfer to complete.

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	freeCommandBuffer(commandBuffer);
}

// Allocates all the commands buffers saved in m_commandBuffers in the cmd pool.

void CommandPool::allocAllCommandBuffers()
{
	if (m_commandBuffers.size() == 0)
		throw std::runtime_error("Allocating empty CMD Buffers to the CMD Pool!");

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

	if (vkAllocateCommandBuffers(m_logicalDevice,&allocInfo,m_commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers!");
}

const VkCommandBuffer& CommandPool::getCommandBuffer(const uint32_t index) 
{
	return m_commandBuffers[index];
}

void CommandPool::recordCommandBuffer(
	const VkFramebuffer& framebuffer,
	const VkRenderPass& renderPass,
	const VkExtent2D& extent,
	const VkPipeline& graphicsPipeline,
	const uint32_t index,
	const VkBuffer& vertexBuffer,
	const VkBuffer& indexBuffer,
	const size_t indexCount,
	const VkPipelineLayout& pipelineLayout,
	const std::vector<VkDescriptorSet>& descriptorSets)
{
	// Specifies some details about the usage of this specific command
	// buffer.
	beginCommandBuffer(0, m_commandBuffers[index]);

		// NUMBER OF VK_ATTACHMENT_LOAD_OP_CLEAR == CLEAR_VALUES
		std::vector<VkClearValue> clearValues(2);
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].color = { 1.0f, 0.0f };

		VkRenderPassBeginInfo renderPassInfo{};
		createRenderPassBeginInfo(renderPass,framebuffer,extent,clearValues,renderPassInfo);


		//--------------------------------RenderPass--------------------------------
		// The final parameter controls how the drawing commands between the
		// render pass will be provided:
		//    -VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be
		//    embedded in the primary command buffer itself and no secondary
		//    command buffers will be executed.
		//    -VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass
		//    commands will be executed from secondary command buffers.

		vkCmdBeginRenderPass(m_commandBuffers[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		CommandUtils::STATE::bindPipeline(graphicsPipeline,m_commandBuffers[index]);
		CommandUtils::STATE::bindVertexBuffers({ vertexBuffer }, { 0 }, 0, 1, m_commandBuffers[index]);
		CommandUtils::STATE::bindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32, m_commandBuffers[index]);

		// Set Dynamic States
		CommandUtils::STATE::setViewport(0.0f, 0.0f, extent, 0.0f, 1.0f, 0, 1, m_commandBuffers[index]);
		CommandUtils::STATE::setScissor({ 0, 0 }, extent, 0, 1, m_commandBuffers[index]);
		CommandUtils::STATE::bindDescriptorSets(pipelineLayout, 0, { descriptorSets[index] }, {}, m_commandBuffers[index]);

		CommandUtils::ACTION::drawIndexed(indexCount, 1, 0, 0, 0, m_commandBuffers[index]);

		vkCmdEndRenderPass(m_commandBuffers[index]);

	endCommandBuffer(m_commandBuffers[index]);
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


void CommandPool::beginCommandBuffer(const VkCommandBufferUsageFlags& flags, VkCommandBuffer& commandBuffer)
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

void CommandPool::endCommandBuffer(VkCommandBuffer& commandBuffer)
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

CommandPool::~CommandPool() {}