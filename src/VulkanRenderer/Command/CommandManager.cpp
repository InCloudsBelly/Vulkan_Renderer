#include "VulkanRenderer/Command/CommandManager.h"

#include <vector>

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
