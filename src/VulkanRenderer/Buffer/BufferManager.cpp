#include "VulkanRenderer/Buffer/BufferManager.h"

#include <stdexcept>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#define VMA_IMPLEMENTATION 
#include <vk_mem_alloc.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <random>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vulkan/vulkan.h>

#include "VulkanRenderer/Model/Attributes.h"
#include "VulkanRenderer/Buffer/BufferUtils.h"
#include "VulkanRenderer/Command/CommandManager.h"

#include "VulkanRenderer/Renderer.h"

const float PI = (float)M_PI;

#define CHECKRESULT(x)          \
    {                             \
        VkResult retval = (x);    \
        if (retval != VK_SUCCESS) \
        {                         \
            return retval;        \
        }                         \
    }

struct Face
{
	float* data;
	int width;
	int height;
	// for mem copy purposes only
	int currentOffset;
};



VkResult BufferManager::bufferCreateBuffer(
	VmaAllocator allocator,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VmaMemoryUsage vmaUsage,
	VkBuffer* buffer,
	VmaAllocation* allocation)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = vmaUsage; //VMA_MEMORY_USAGE_GPU_ONLY;

	return vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, buffer, allocation, nullptr);
}

/**
* \brief Copy a buffer from a source buffer to a destination buffer
*
* \param[in] device Logical Vulkan device
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command bbuffers
* \param[in] srcBuffer Source buffer
* \param[in] dstBuffer Destination buffer
* \param[in] size Size in bytes of the data to be copied
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCopyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = CommandManager::cmdBeginSingleTimeCommands(device, commandPool);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	return CommandManager::cmdEndSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
}




VkResult BufferManager::createSharedConcurrentBuffer(
	VmaAllocator allocator,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	QueueFamilyIndices queueFamilyIndices,
	VmaMemoryUsage vmaUsage,
	VkBuffer* buffer,
	VmaAllocation* allocation
)
{
	std::vector<uint32_t> necessaryIndices;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.queueFamilyIndexCount = 2;

	if (queueFamilyIndices.graphicsFamily.has_value() && queueFamilyIndices.computeFamily.has_value())
	{
		necessaryIndices = { queueFamilyIndices.graphicsFamily.value(),queueFamilyIndices.computeFamily.value() };

		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bufferInfo.pQueueFamilyIndices = necessaryIndices.data();
	}
	else
	{
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.pQueueFamilyIndices = nullptr;
	}

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = vmaUsage; //VMA_MEMORY_USAGE_GPU_ONLY;

	return vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, buffer, allocation, nullptr);
}



/**
* \returns whether a depth image format supports stencil information
*/
bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

/**
* \brief Copy a buffer to an image (for uploading via a staging buffer)
*
* \param[in] device Logical Vulkan device
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command buffers
* \param[in] buffer The source buffer
* \param[in] image The destination image
* \param[in] layerCount Number of layers in the image
* \param[in] width Image width
* \param[in] height Image height
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCopyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t layerCount, uint32_t width, uint32_t height)
{
	std::vector<VkBufferImageCopy> regions;

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };
	regions.push_back(region);

	return BufferManager::bufferCopyBufferToImage(device, graphicsQueue, commandPool, buffer, image, regions, width, height);
}

/**
* \brief Wrapper for copying a buffer to an image, manages queue submission
*
* \param[in] device Logical Vulkan device
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command buffers
* \param[in] buffer The source buffer
* \param[in] image The destination image
* \param[in] regions The regions to be copied
* \param[in] width Width of the image
* \param[in] height Height of the image
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCopyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, std::vector<VkBufferImageCopy>& regions, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = CommandManager::cmdBeginSingleTimeCommands(device, commandPool);

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)regions.size(), regions.data());

	return CommandManager::cmdEndSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
}

/**
* \brief Copy a VKImage to a VKBuffer
*
* \param[in] device Logical Vulkan device
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command buffers
* \param[in] image The source image
* \param[in] aspect Aspect on how to use the image (color or depth)
* \param[in] buffer The destination buffer
* \param[in] layerCount Number of image layers
* \param[in] width �mage width
* \param[in] height Image height
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCopyImageToBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkImageAspectFlagBits aspect, VkBuffer buffer, uint32_t layerCount, uint32_t width, uint32_t height)
{
	std::vector<VkBufferImageCopy> regions;

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = aspect;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };
	regions.push_back(region);

	return BufferManager::bufferCopyImageToBuffer(device, graphicsQueue, commandPool, image, buffer, regions, width, height);
}

/**
* \brief Copy a VKImage to a VKBuffer
*
* \param[in] device Logical Vulkan device
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command buffers
* \param[in] image The source image
* \param[in] buffer The destination buffer
* \param[in] regions Copy regions detailing the image parts that should be copied
* \param[in] width �mage width
* \param[in] height Image height
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCopyImageToBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkBuffer buffer, std::vector<VkBufferImageCopy>& regions, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = CommandManager::cmdBeginSingleTimeCommands(device, commandPool);

	vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, (uint32_t)regions.size(), regions.data());

	return CommandManager::cmdEndSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
}

//-------------------------------------------------------------------------------------------------------
//
/**
* \brief Transition the image layout, causes a pipeline barrier
*
* \param[in] device Logical Vulkan device
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command buffers
* \param[in] image The destination image
* \param[in] format The Image format
* \param[in] aspect Color or depth
* \param[in] miplevels Number of image miplevels
* \param[in] layerCount Number of image layers
* \param[in] oldLayout Old layout of the image
* \param[in] newLayout The layout should be transitioned to this new layout
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferTransitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageAspectFlagBits aspect, uint32_t miplevels, uint32_t layerCount, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = CommandManager::cmdBeginSingleTimeCommands(device, commandPool);

	CHECKRESULT(
		BufferManager::bufferTransitionImageLayout(
			device,
			graphicsQueue,
			commandBuffer,
			image,
			format, 
			aspect, 
			miplevels, 
			layerCount,
			oldLayout, 
			newLayout
		)
	);

	return CommandManager::cmdEndSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
}

/**
* \brief Transition the image layout, causes a pipeline barrier
*
* \param[in] device Logical Vulkan device
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandBuffer Command buffer to record this operation into
* \param[in] image The destination image
* \param[in] format The Image format
* \param[in] aspect Color or depth
* \param[in] miplevels Number of image miplevels
* \param[in] layerCount Number of image layers
* \param[in] oldLayout Old layout of the image
* \param[in] newLayout The layout should be transitioned to this new layout
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferTransitionImageLayout(
	VkDevice device, 
	VkQueue graphicsQueue, 
	VkCommandBuffer commandBuffer, 
	VkImage image, 
	VkFormat format, 
	VkImageAspectFlagBits aspect,
	uint32_t miplevels, 
	uint32_t layerCount,
	VkImageLayout oldLayout,
	VkImageLayout newLayout
)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect;

	if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT && hasStencilComponent(format))
	{
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = miplevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = (VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);

		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		throw std::invalid_argument("Unsupported layout transition!");

	vkCmdPipelineBarrier(commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	return VK_SUCCESS;
}


/**
* \brief Create framebuffers (color + depth), one for each swap chain image
*
* \param[in] device Logical Vulkan device
* \param[in] imageViews Color images from the swap chain
* \param[in] depthImageViews List with views of the depth images
* \param[in] renderPass Render pass to be used in
* \param[in] extent Extent of the swap chain images
* \param[out] frameBuffers The resulting frame buffers
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCreateFramebuffers(VkDevice device,
	std::vector<VkImageView> imageViews,
	std::vector<VkImageView> depthImageViews, //should have same length!
	VkRenderPass renderPass,
	VkExtent2D extent,
	std::vector<VkFramebuffer>& frameBuffers)
{
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;

	uint32_t loops = (uint32_t)std::max(imageViews.size(), depthImageViews.size());
	frameBuffers.resize(loops);

	for (uint32_t i = 0; i < loops; i++)
	{
		std::vector<VkImageView> attachments;

		if (imageViews.size() > i && imageViews[i] != VK_NULL_HANDLE)
			attachments.push_back(imageViews[i]);

		if (depthImageViews.size() > i && depthImageViews[i] != VK_NULL_HANDLE)
			attachments.push_back(depthImageViews[i]);

		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();

		CHECKRESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffers[i]));
	}
	return VK_SUCCESS;
}

/**
* \brief Create framebuffers (position, normal, albedo, depth), one for each swap chain image
* \param[in] device Logical Vulkan device
* \param[in] imageViews Color images from the swap chain
* \param[in] positionImageViews List with views of the position images
* \param[in] normalImageViews List with views of the normal images
* \param[in] albedoImageViews List with views of the albedo images
* \param[in] depthImageViews List with views of the depth images
* \param[in] renderPass Render pass to be used in
* \param[in] extent Extent of the swap chain images
* \param[out] frameBuffers The resulting frame buffers
*/
VkResult BufferManager::bufferCreateFramebuffersOffscreen(VkDevice device,
	std::vector<VkImageView> positionImageViews,
	std::vector<VkImageView> normalImageViews, //should have same length!
	std::vector<VkImageView> albedoImageViews, //should have same length!
	std::vector<VkImageView> depthImageViews, //should have same length!
	VkRenderPass renderPass,
	VkExtent2D extent,
	std::vector<VkFramebuffer>& frameBuffers)
{
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;

	uint32_t loops = (uint32_t)std::max(positionImageViews.size(), depthImageViews.size());
	frameBuffers.resize(loops);

	for (uint32_t i = 0; i < loops; i++)
	{
		std::vector<VkImageView> attachments;

		if (positionImageViews.size() > i && positionImageViews[i] != VK_NULL_HANDLE)
			attachments.push_back(positionImageViews[i]);
		if (normalImageViews.size() > i && normalImageViews[i] != VK_NULL_HANDLE)
			attachments.push_back(normalImageViews[i]);
		if (albedoImageViews.size() > i && albedoImageViews[i] != VK_NULL_HANDLE)
			attachments.push_back(albedoImageViews[i]);
		if (depthImageViews.size() > i && depthImageViews[i] != VK_NULL_HANDLE)
			attachments.push_back(depthImageViews[i]);

		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
	return VK_SUCCESS;
}

/**
* \brief Copy a swap chain image to a data buffer, after it has been rendered into
*
* \param[in] device Logical Vulkan device
* \param[in] allocator VMA allocator
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command buffers
* \param[in] image The source image
* \param[in] format The pixel format of this image
* \param[in] aspect Color or depth
* \param[in] layout The layout that this image is currently and should be again after the copy
* \param[in] bufferData The destination buffer data
* \param[in] width �mage width
* \param[in] height Image height
* \param[in] imageSize Size of the image in bytes
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCopySwapChainImageToHost(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageAspectFlagBits aspect, VkImageLayout layout,
	/*gli::byte*/ unsigned char* bufferData,
	uint32_t width,
	uint32_t height,
	uint32_t imageSize)
{
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	CHECKRESULT(BufferManager::bufferCreateBuffer(allocator, imageSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
		&stagingBuffer, &stagingBufferAllocation));

	CHECKRESULT(BufferManager::bufferTransitionImageLayout(device, graphicsQueue, commandPool,
		image, format, aspect, 1, 1,
		layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));

	CHECKRESULT(BufferManager::bufferCopyImageToBuffer(device, graphicsQueue, commandPool,
		image, aspect, stagingBuffer, 1, width, height));

	CHECKRESULT(BufferManager::bufferTransitionImageLayout(device, graphicsQueue, commandPool,
		image, format, aspect, 1, 1,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, layout));

	void* data;
	CHECKRESULT(vmaMapMemory(allocator, stagingBufferAllocation, &data));
	memcpy(bufferData, data, (uint32_t)imageSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	for (uint32_t i = 0; i < width * height; i++)
	{
		/*gli::byte*/ unsigned char r = bufferData[4 * i + 0];
		/*gli::byte*/ unsigned char g = bufferData[4 * i + 1];
		/*gli::byte*/ unsigned char b = bufferData[4 * i + 2];
		/*gli::byte*/ unsigned char a = bufferData[4 * i + 3];

		bufferData[4 * i + 0] = b;
		bufferData[4 * i + 1] = g;
		bufferData[4 * i + 2] = r;
		bufferData[4 * i + 3] = a;
	}

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
	return VK_SUCCESS;
}

/**
* \brief Copy a swap chain image to a data buffer, after it has been rendered into
*
* \param[in] device Logical Vulkan device
* \param[in] allocator VMA allocator
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command buffers
* \param[in] image The source image
* \param[in] format Format of the image
* \param[in] aspect Color or depth
* \param[in] layout The layout the image is currently in
* \param[in] bufferData The destination buffer data
* \param[in] width �mage width
* \param[in] height Image height
* \param[in] imageSize Size of the image in bytes
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCopyImageToHost(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageAspectFlagBits aspect, VkImageLayout layout,
	/*gli::byte*/ unsigned char* bufferData,
	uint32_t width,
	uint32_t height,
	uint32_t imageSize)
{
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	CHECKRESULT(BufferManager::bufferCreateBuffer(allocator, imageSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
		&stagingBuffer, &stagingBufferAllocation));

	CHECKRESULT(BufferManager::bufferTransitionImageLayout(device, graphicsQueue, commandPool,
		image, format, aspect, 1, 1,
		layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));

	CHECKRESULT(BufferManager::bufferCopyImageToBuffer(device, graphicsQueue, commandPool,
		image, aspect, stagingBuffer, 1, width, height));

	CHECKRESULT(BufferManager::bufferTransitionImageLayout(device, graphicsQueue, commandPool,
		image, format, aspect, 1, 1,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, layout));

	void* data;
	CHECKRESULT(vmaMapMemory(allocator, stagingBufferAllocation, &data));
	memcpy(bufferData, data, (uint32_t)imageSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------
//create main entity buffers

/**
* \brief Create a Vulkan vertex buffer
*
* \param[in] device Logical Vulkan device
* \param[in] allocator VMA allocator
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command bbuffers
* \param[in] vertices List of vertices and their data
* \param[out] vertexBuffer The new vertex buffer
* \param[out] vertexBufferAllocation VMA allocation information
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCreateVertexBuffer(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, std::vector<MeshVertex>& vertices, VkBuffer* vertexBuffer, VmaAllocation* vertexBufferAllocation)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	CHECKRESULT(BufferManager::bufferCreateBuffer(allocator, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
		&stagingBuffer, &stagingBufferAllocation));

	void* data;
	CHECKRESULT(vmaMapMemory(allocator, stagingBufferAllocation, &data));
	memcpy(data, vertices.data(), (uint32_t)bufferSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	CHECKRESULT(BufferManager::bufferCreateBuffer(allocator, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer, vertexBufferAllocation));

	CHECKRESULT(BufferManager::bufferCopyBuffer(device, graphicsQueue, commandPool, stagingBuffer, *vertexBuffer, bufferSize));

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
	return VK_SUCCESS;
}


VkResult BufferManager::createBufferAndTransferToDevice(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, void* vertices, uint32_t size, VkBufferUsageFlags usageDstBuffer, VkBuffer* vertexBuffer, VmaAllocation* vertexBufferAllocation)
{
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	CHECKRESULT(BufferManager::bufferCreateBuffer(allocator, size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
		&stagingBuffer, &stagingBufferAllocation));

	void* data;
	CHECKRESULT(vmaMapMemory(allocator, stagingBufferAllocation, &data));
	memcpy(data, vertices, size);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	CHECKRESULT(BufferManager::bufferCreateBuffer(allocator, size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageDstBuffer |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer, vertexBufferAllocation));

	CHECKRESULT(BufferManager::bufferCopyBuffer(device, graphicsQueue, commandPool, stagingBuffer, *vertexBuffer, size));

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
	return VK_SUCCESS;
}

/**
* \brief Create a Vulkan index buffer
*
* \param[in] device Logical Vulkan device
* \param[in] allocator VMA allocator
* \param[in] graphicsQueue Device queue for submitting commands
* \param[in] commandPool Command pool for allocating command bbuffers
* \param[in] indices List of indices
* \param[out] indexBuffer The new index buffer
* \param[out] indexBufferAllocation VMA allocation information
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult
BufferManager::bufferCreateIndexBuffer(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkCommandPool commandPool, std::vector<uint32_t>& indices, VkBuffer* indexBuffer, VmaAllocation* indexBufferAllocation)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	CHECKRESULT(
		BufferManager::bufferCreateBuffer(allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
			&stagingBuffer, &stagingBufferAllocation));

	void* data;
	CHECKRESULT(vmaMapMemory(allocator, stagingBufferAllocation, &data));
	memcpy(data, indices.data(), (uint32_t)bufferSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	CHECKRESULT(BufferManager::bufferCreateBuffer(allocator, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		indexBuffer, indexBufferAllocation));

	CHECKRESULT(BufferManager::bufferCopyBuffer(device, graphicsQueue, commandPool, stagingBuffer, *indexBuffer, bufferSize));

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
	return VK_SUCCESS;
}

/**
* \brief Create a UBO
*
* \param[in] allocator VMA allocator
* \param[in] numberBuffers The number of UBOs to be created
* \param[in] bufferSize Size of each new buffer
* \param[out] uniformBuffers List containing the new buffers
* \param[out] uniformBuffersAllocation VMA allocation information
* \returns VK_SUCCESS or a Vulkan error code
*
*/
VkResult BufferManager::bufferCreateUniformBuffers(
	VmaAllocator allocator,
	uint32_t numberBuffers,
	VkDeviceSize bufferSize,
	std::vector<VkBuffer>& uniformBuffers,
	std::vector<VmaAllocation>& uniformBuffersAllocation)
{
	uniformBuffers.resize(numberBuffers);
	uniformBuffersAllocation.resize(numberBuffers);

	for (uint32_t i = 0; i < numberBuffers; i++)
	{
		CHECKRESULT(
			BufferManager::bufferCreateBuffer(
				allocator,
				bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				&uniformBuffers[i],
				&uniformBuffersAllocation[i]
			)
		);
	}
	return VK_SUCCESS;
}
