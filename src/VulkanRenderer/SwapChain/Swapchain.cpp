#include "VulkanRenderer/Swapchain/Swapchain.h"

#include <algorithm>
#include <iostream>

#include <vulkan/vulkan.h>
#include <stdexcept>

#include "VulkanRenderer/Queue/QueueFamilyIndices.h"
#include "VulkanRenderer/Images/ImageManager.h"
#include "VulkanRenderer/Window/Window.h"
#include "VulkanRenderer/Features/MSAA.h"
#include "VulkanRenderer/Features/DepthBuffer.h"
#include "VulkanRenderer/Framebuffer/FramebufferUtils.h"

Swapchain::Swapchain() {}
Swapchain::~Swapchain() {}

Swapchain::Swapchain(
	const VkPhysicalDevice& physicalDevice,
	const VkDevice& logicalDevice,
	const std::shared_ptr<Window>& window,
	const SwapchainSupportedProperties& supportedProperties )
	: m_logicalDevice(logicalDevice)
{
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;

	chooseBestSettings(window, supportedProperties, surfaceFormat, presentMode, extent);

	m_imageFormat = surfaceFormat.format;
	m_extent = extent;
	
	// Chooses how many images we want to have in the swap chain.
	// (It's always recommended to request at least one more image that the
	// minimum because if we stick to this minimum, it means that we may
	// sometimes have to wait on the drive to complete internal operations
	// before we can acquire another imager to render to)
	m_minImageCount = (supportedProperties.capabilities.minImageCount);
	uint32_t imageCount = m_minImageCount + 1;

	bool isMaxResolution = existsMaxNumberOfSupportedImages(supportedProperties.capabilities);
	if (isMaxResolution == true &&imageCount > supportedProperties.capabilities.maxImageCount)
	{
		imageCount = supportedProperties.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = window->getSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices;
	indices.getIndicesOfRequiredQueueFamilies(physicalDevice,window->getSurface());
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),indices.presentFamily.value() };
	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		// Optinal
		createInfo.queueFamilyIndexCount = 0;
		// Optional
		createInfo.pQueueFamilyIndices = nullptr;
	}
	// We can specify that a certain transform should be applied to images in
	// the swap chain if it's supported(supportedTransofrms in capabilities),
	// like a 90 degree clockwsie rotation or horizontal flip.
	createInfo.preTransform = (supportedProperties.capabilities.currentTransform);

	// Specifies if the alpha channel should be used for mixing with other
   // windows in the window sysem. We'll almost always want to simply ignore
   // the alpha channel:
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	// We don't care abou the color pixels that are obscured, for example
	// because another window is in front of them. Unless you really need to be
	// able to read these pixels back an get predictable results, we'll get the
	// best performance by enabling clipping.
	createInfo.clipped = VK_TRUE;

	// Configures the old swapchain when the actual one becomes invaled or
	// unoptimized while the app is running(for example because the window was
	// resized).
	// For now we'll assume that we'll only ever create one swapchain.
	createInfo.oldSwapchain = VK_NULL_HANDLE;


	if (vkCreateSwapchainKHR(logicalDevice,&createInfo,nullptr,&m_swapchain) != VK_SUCCESS)
		throw std::runtime_error("Failed to create the Swapchain!");

	vkGetSwapchainImagesKHR(logicalDevice, m_swapchain, &imageCount, nullptr);
	m_images.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, m_swapchain, &imageCount, m_images.data());
}

void Swapchain::destroy()
{
	for (auto& framebuffer : m_framebuffers)
		vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);

	vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);

	for (auto& imageView : m_imageViews)
		vkDestroyImageView(m_logicalDevice, imageView, nullptr);
}


void Swapchain::createAllImageViews()
{
	m_imageViews.resize(m_images.size());

	for (size_t i = 0; i < m_images.size(); i++)
	{
		ImageManager::createImageView(
			m_logicalDevice,
			m_imageFormat,
			m_images[i],
			VK_IMAGE_ASPECT_COLOR_BIT,
			false,
			1,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			m_imageViews[i]
		);
	}
}

void Swapchain::createFramebuffers(const VkRenderPass& renderPass,const DepthBuffer& depthBuffer,const MSAA& msaa)
{
	m_framebuffers.resize(m_imageViews.size());

	for (size_t i = 0; i < m_imageViews.size(); i++)
	{
		// Images in which we'll write in.
		std::vector<VkImageView> attachments = {
			msaa.getImageView(),
			depthBuffer.getImageView(),
			m_imageViews[i]
		};

		FramebufferUtils::createFramebuffer(
			m_logicalDevice, 
			renderPass,
			attachments,
			m_extent.width,
			m_extent.height,
			1,
			m_framebuffers[i]
		); 
	}
}

const VkImageView& Swapchain::getImageView(const uint32_t index) const {
	return m_imageViews[index];
}

void Swapchain::chooseBestSettings(
	const std::shared_ptr<Window>& window,
	const SwapchainSupportedProperties& supportedProperties,
	VkSurfaceFormatKHR& surfaceFormat,
	VkPresentModeKHR& presentMode,
	VkExtent2D& extent)
{
	surfaceFormat = chooseBestSurfaceFormat(supportedProperties.surfaceFormats);
	presentMode = chooseBestPresentMode(supportedProperties.presentModes);
	extent = chooseBestExtent(supportedProperties.capabilities,window);
}


VkSurfaceFormatKHR Swapchain::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
{
	for (const auto& availableFormat : availableFormats)
	{
		// sRGB -> gamma correction.
		if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseBestPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseBestExtent(const VkSurfaceCapabilitiesKHR& capabilities, const std::shared_ptr<Window>& window)
{
	if (window->isAllowedToModifyTheResolution(capabilities) == false)
		return capabilities.currentExtent;

	int width, height;
	window->getResolutionInPixels(width, height);

	VkExtent2D actualExtent = { static_cast<uint32_t>(width),static_cast<uint32_t>(height) };

	actualExtent.width = std::clamp(
		actualExtent.width,
		capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width
	);
	actualExtent.height = std::clamp(
		actualExtent.height,
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height
	);


	return actualExtent;
}

const VkExtent2D& Swapchain::getExtent() const
{
	return m_extent;
}

const VkFramebuffer& Swapchain::getFramebuffer(const uint32_t imageIndex) const
{
	return m_framebuffers[imageIndex];
}

const VkFormat& Swapchain::getImageFormat() const
{
	return m_imageFormat;
}

const VkSwapchainKHR& Swapchain::get() const
{
	return m_swapchain;
}

const bool Swapchain::existsMaxNumberOfSupportedImages(const VkSurfaceCapabilitiesKHR& capabilities) const
{
	return (capabilities.maxImageCount != 0);
}

const uint32_t Swapchain::getImageCount() const
{
	return m_images.size();
}

const uint32_t Swapchain::getMinImageCount() const
{
	return m_minImageCount;
}
