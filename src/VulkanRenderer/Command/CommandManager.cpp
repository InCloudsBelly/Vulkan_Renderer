#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/Queue/QueueFamilyIndices.h"
#include <vector>

#define CHECKRESULT(x)          \
    {                             \
        VkResult retval = (x);    \
        if (retval != VK_SUCCESS) \
        {                         \
            return retval;        \
        }                         \
    }


//////////////////////////////////ACTION CMDs//////////////////////////////////

void CommandManager::ACTION::copyBufferToImage(
    const VkBuffer& srcBuffer,
    const VkImage& dstImage,
    const VkImageLayout& dstImageLayout,
    const uint32_t& regionCount,
    const VkBufferImageCopy& regions,
    const VkCommandBuffer& commandBuffer ) 
{
    vkCmdCopyBufferToImage(commandBuffer,srcBuffer,dstImage,dstImageLayout,regionCount,&regions);
}

void CommandManager::ACTION::copyBufferToBuffer(
    const VkBuffer& srcBuffer,
    const VkBuffer& dstBuffer,
    const uint32_t& regionCount,
    const VkBufferCopy& regions,
    const VkCommandBuffer& commandBuffer ) 
{
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, &regions);
}


void CommandManager::ACTION::drawIndexed(
    const uint32_t& indexCount,
    const uint32_t& instanceCount,
    const uint32_t& firstIndex,
    const uint32_t& vertexOffset,
    const uint32_t& firstInstance,
    const VkCommandBuffer& commandBuffer ) 
{
    vkCmdDrawIndexed(commandBuffer,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
}


void CommandManager::ACTION::dispatch(
    const uint32_t& xSize,
    const uint32_t& ySize,
    const uint32_t& zSize,
    const VkCommandBuffer& commandBuffer
) {
    vkCmdDispatch(commandBuffer, xSize, ySize, zSize);
}


//////////////////////////////////////STATE////////////////////////////////////

void CommandManager::STATE::bindPipeline(
    const VkPipeline& pipeline,
    const PipelineType& pipelineType,
    const VkCommandBuffer& commandBuffer
) {
    vkCmdBindPipeline(commandBuffer,
        // TODO: make it a separated function to run it faster?
        ((pipelineType == PipelineType::GRAPHICS) ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE),
        pipeline
    );
}

void CommandManager::STATE::bindVertexBuffers(
    const std::vector<VkBuffer>& vertexBuffers,
    const std::vector<VkDeviceSize>& offsets,
    const uint32_t& indexOfFirstBinding,
    const uint32_t& bindingCount,
    const VkCommandBuffer& commandBuffer
) {
    vkCmdBindVertexBuffers(commandBuffer, indexOfFirstBinding, bindingCount, vertexBuffers.data(), offsets.data());
}

void CommandManager::STATE::bindIndexBuffer(
    const VkBuffer& indexBuffer,
    const VkDeviceSize& offset,
    const VkIndexType& indexType,
    const VkCommandBuffer& commandBuffer
) {

    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void CommandManager::STATE::setViewport(
    const float& x,
    const float& y,
    const VkExtent2D& extent,
    const float& minDepth,
    const float& maxDepth,
    const uint32_t& firstViewport,
    const uint32_t& viewportCount,
    const VkCommandBuffer& commandBuffer
) {
    VkViewport viewport{};
    viewport.x = x;
    viewport.y = y;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;

    vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, &viewport);
}

void CommandManager::STATE::setScissor(
    const VkOffset2D& offset,
    const VkExtent2D& extent,
    const uint32_t& firstScissor,
    const uint32_t& scissorCount,
    const VkCommandBuffer& commandBuffer
) {
    VkRect2D scissor{};
    scissor.offset = offset;
    scissor.extent = extent;

    vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, &scissor);
}

void CommandManager::STATE::bindDescriptorSets(
    const VkPipelineLayout& pipelineLayout,
    const PipelineType& pipelineType,
    const uint32_t& firstSet,
    const std::vector<VkDescriptorSet>& descriptorSets,
    const std::vector<uint32_t>& dynamicOffsets,
    const VkCommandBuffer& commandBuffer
) {
    vkCmdBindDescriptorSets(
        commandBuffer,
        ((pipelineType == PipelineType::GRAPHICS) ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE),
        pipelineLayout,
        // Index of the first descriptor set.
        firstSet,
        descriptorSets.size(),
        descriptorSets.data(),
        dynamicOffsets.size(),
        dynamicOffsets.data()
    );
}




////////////////////////////////////Sync. CMDs/////////////////////////////////


void CommandManager::SYNCHRONIZATION::recordPipelineBarrier(
    const VkPipelineStageFlags& srcStageFlags,
    const VkPipelineStageFlags& dstStageFlags,
    const VkDependencyFlags& dependencyFlags,
    const VkCommandBuffer& commandBuffer,
    const std::vector<VkMemoryBarrier>& memoryBarriers,
    const std::vector<VkBufferMemoryBarrier>& bufferMemoryBarriers,
    const std::vector<VkImageMemoryBarrier>& imageMemoryBarriers
) {

    vkCmdPipelineBarrier(
        commandBuffer,
        // Pipeline stage in which operations occur that should happen 
        // before the barrier.
        srcStageFlags,
        // Pipeline stage in which operations will wait on the barrier.
        dstStageFlags,
        // 0 or VK_DEPENDENCY_BY_REGION_BIT(per-region condition)
        dependencyFlags,
        // References arrays of pipeline barries of the three available
        // types: memory barriers, buffer memory barriers, and image memory
        // barriers.
        memoryBarriers.size(), memoryBarriers.data(),
        bufferMemoryBarriers.size(), bufferMemoryBarriers.data(),
        imageMemoryBarriers.size(), imageMemoryBarriers.data()
    );
}





VkResult CommandManager::cmdCreateCommandPool(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkCommandPool* commandPool)
{
	QueueFamilyIndices queueFamilyIndices;
	queueFamilyIndices.getIndicesOfRequiredQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	return vkCreateCommandPool(device, &poolInfo, nullptr, commandPool);
}

//-------------------------------------------------------------------------------------------------------

/**
	*
	* \brief Create a number of command buffers
	*
	* \param[in] device Logical Vulkan device
	* \param[in] commandPool Command pool for allocating command bbuffers
	* \param[in] level Level can be primary or secondary
	* \param[in] count Number of buffers to create
	* \param[in] pBuffers Pointer to an array where the buffer handles should be stored
	* \returns VK_SUCCESS or a Vulkan error code
	*
	*/
VkResult CommandManager::cmdCreateCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count, VkCommandBuffer* pBuffers)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = level;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = count;

	return vkAllocateCommandBuffers(device, &allocInfo, pBuffers);
}

/**
	*
	* \brief Start a command buffer for recording commands
	*
	* \param[in] device Logical Vulkan device
	* \param[in] renderPass The render pass that is inherited from the parent command buffer
	* \param[in] subpass Index of subpass
	* \param[in] frameBuffer The framebuffer that is rendered into
	* \param[in] commandBuffer The command buffer to start
	* \param[in] usageFlags Flags telling how the buffer will be used
	* \returns VK_SUCCESS or a Vulkan error code
	*
	*/
VkResult
CommandManager::cmdBeginCommandBuffer(VkDevice device, VkRenderPass renderPass, uint32_t subpass, VkFramebuffer frameBuffer, VkCommandBuffer commandBuffer, VkCommandBufferUsageFlagBits usageFlags)
{
	VkCommandBufferInheritanceInfo inheritance = {};
	inheritance.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritance.framebuffer = frameBuffer;
	inheritance.renderPass = renderPass;
	inheritance.subpass = subpass;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = usageFlags;
	beginInfo.pInheritanceInfo = &inheritance;

	return vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

/**
	*
	* \brief Start a command buffer for recording commands
	*
	* \param[in] device Logical Vulkan device
	* \param[in] commandBuffer The command buffer to start
	* \param[in] usageFlags Flags telling how the buffer will be used
	* \returns VK_SUCCESS or a Vulkan error code
	*
	*/
VkResult CommandManager::cmdBeginCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkCommandBufferUsageFlagBits usageFlags)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = usageFlags;

	return vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

/**
	*
	* \brief Submit a command buffer to a queue
	*
	* \param[in] device Logical Vulkan device
	* \param[in] queue The queue the buffer is sent to
	* \param[in] commandBuffer The command buffer that is sent to the queue
	* \param[in] waitSemaphore A semaphore to wait for before submitting
	* \param[in] signalSemaphore Signal this semaphore after buffer is done
	* \param[in] waitFence Signal to this fence after buffer is done
	* \returns VK_SUCCESS or a Vulkan error code
	*
	*/
VkResult CommandManager::cmdSubmitCommandBuffer(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence waitFence)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { waitSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	if (waitSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
	}

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore signalSemaphores[] = { signalSemaphore };
	if (signalSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
	}

	if (waitFence != VK_NULL_HANDLE)
	{
		vkResetFences(device, 1, &waitFence);
	}

	return vkQueueSubmit(queue, 1, &submitInfo, waitFence);
}

/**
	*
	* \brief Begin submitting a single time command
	*
	* \param[in] device Logical Vulkan device
	* \param[in] commandPool Command pool for allocating command bbuffers
	* \returns a new VkCommandBuffer to record commands into
	*
	*/
VkCommandBuffer CommandManager::cmdBeginSingleTimeCommands(VkDevice device,  VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		return VK_NULL_HANDLE;

	return commandBuffer;
}

/**
	*
	* \brief End recording into a single time command buffer and submit it
	*
	* \param[in] device Logical Vulkan device
	* \param[in] graphicsQueue Queue to submit the buffer to
	* \param[in] commandPool Give back the command buffer to the pool
	* \param[in] commandBuffer The ready to be used command buffer
	* \returns VK_SUCCESS or a Vulkan error code
	*
	*/
VkResult CommandManager::cmdEndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
	VkFence waitFence;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	vkCreateFence(device, &fenceInfo, nullptr, &waitFence);

	VkResult result = CommandManager::cmdEndSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer,
		VK_NULL_HANDLE, VK_NULL_HANDLE, waitFence);

	vkDestroyFence(device, waitFence, nullptr);

	return result;
}

/**
	*
	* \brief End recording into a single time command buffer and submit it
	*
	* \param[in] device Logical Vulkan device
	* \param[in] graphicsQueue Queue to submit the buffer to
	* \param[in] commandPool Give back the command buffer to the pool
	* \param[in] commandBuffer The ready to be used command buffer
	* \param[in] waitSemaphore A semaphore to wait for before submitting
	* \param[in] signalSemaphore Signal this semaphore after buffer is done
	* \param[in] waitFence Signal to this fence after buffer is done
	* \returns VK_SUCCESS or a Vulkan error code
	*
	*/
VkResult CommandManager::cmdEndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence waitFence)
{
	CHECKRESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { waitSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	if (waitSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
	}

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore signalSemaphores[] = { signalSemaphore };
	if (signalSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
	}

	if (waitFence != VK_NULL_HANDLE)
	{
		vkResetFences(device, 1, &waitFence);
	}

	CHECKRESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, waitFence));

	if (waitFence != VK_NULL_HANDLE)
	{
		CHECKRESULT(vkWaitForFences(device, 1, &waitFence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
	}
	else
		CHECKRESULT(vkQueueWaitIdle(graphicsQueue));

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

	return VK_SUCCESS;
}

