#include "VulkanRenderer/SwapChain/SwapChainManager.h"

#include <algorithm>

#include <vulkan/vulkan.h>
#include <stdexcept>

#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"

SwapchainManager::SwapchainManager() {}

void SwapchainManager::createSwapchain(
	const VkPhysicalDevice& physicalDevice,
	const VkDevice& logicalDevice,
	const WindowManager& windowM)
{
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;

	chooseBestSettings(
		physicalDevice,
		windowM,
		surfaceFormat,
		presentMode,
		extent
	);

	m_imageFormat = surfaceFormat.format;
	m_extent = extent;
	
	// Chooses how many images we want to have in the swap chain.
	// (It's always recommended to request at least one more image that the
	// minimum because if we stick to this minimum, it means that we may
	// sometimes have to wait on the drive to complete internal operations
	// before we can acquire another imager to render to)
	uint32_t imageCount = m_supportedProperties.value().capabilities.minImageCount + 1;

	bool isMaxResolution = existsMaxNumberOfSupportedImages(m_supportedProperties.value().capabilities);
	if (isMaxResolution == true &&imageCount > m_supportedProperties.value().capabilities.maxImageCount)
	{
		imageCount = m_supportedProperties.value().capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = windowM.getSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices;
	indices.getIndicesOfRequiredQueueFamilies(physicalDevice,windowM.getSurface());
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
	createInfo.preTransform = (m_supportedProperties.value().capabilities.currentTransform);

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

void SwapchainManager::destroySwapchain(const VkDevice& logicalDevice)
{
	vkDestroySwapchainKHR(logicalDevice, m_swapchain, nullptr);
}

void SwapchainManager::destroyImageViews(const VkDevice& logicalDevice)
{
	for (auto& imageView : m_imageViews)
		vkDestroyImageView(logicalDevice, imageView, nullptr);
}

void SwapchainManager::createImageViews(const VkDevice& logicalDevice)
{
	m_imageViews.resize(m_images.size());

	for (size_t i = 0; i < m_images.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_images[i];
		// Specifies how to treat images, as 1D textures, 2D textures, 3D
		// textures and cube maps.
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_imageFormat;
		// Specifies how we want to map all the color channels of the images
		// (E.g: map all of the channels to the red channel for a monochrome
		// texture)
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		// Specifies what the image's purpose is and which part of the image
		// should be accessed.
		// (E.g: with mipmapping leves or multiple layers)
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		const auto status = vkCreateImageView(
			logicalDevice,
			&createInfo,
			nullptr,
			&m_imageViews[i]
		);

		if (status != VK_SUCCESS)
			throw std::runtime_error("Failed to create image views!");
	}
}

SwapchainSupportedProperties SwapchainManager::getSupportedProperties(const VkPhysicalDevice& physicalDevice,const VkSurfaceKHR& surface) 
{
	SwapchainSupportedProperties supported;

	// - Surface capabilities.
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &supported.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		supported.surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, supported.surfaceFormats.data());
	}

	// - Surface Presentation Modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		supported.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, supported.presentModes.data());
	}
	return supported;
}


void SwapchainManager::chooseBestSettings(
	const VkPhysicalDevice& physicalDevice,
	const WindowManager& windowM,
	VkSurfaceFormatKHR& surfaceFormat,
	VkPresentModeKHR& presentMode,
	VkExtent2D& extent)
{
	if (m_supportedProperties.has_value() == false)
	{
		m_supportedProperties = getSupportedProperties(physicalDevice, windowM.getSurface());
	}

	surfaceFormat = chooseBestSurfaceFormat(m_supportedProperties.value().surfaceFormats);
	presentMode = chooseBestPresentMode(m_supportedProperties.value().presentModes);
	extent = chooseBestExtent(m_supportedProperties.value().capabilities,windowM);
}


VkSurfaceFormatKHR SwapchainManager::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR SwapchainManager::chooseBestPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainManager::chooseBestExtent(const VkSurfaceCapabilitiesKHR& capabilities,const WindowManager& windowM) 
{
	if (windowM.isAllowedToModifyTheResolution(capabilities) == false)
		return capabilities.currentExtent;

	int width, height;
	windowM.getResolutionInPixels(width, height);

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

bool SwapchainManager::isSwapchainAdequated(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface)
{
	if (m_supportedProperties.has_value() == false)
		m_supportedProperties = getSupportedProperties(physicalDevice, surface);

	return (m_supportedProperties.value().surfaceFormats.empty() != true && m_supportedProperties.value().presentModes.empty() != true);
}

bool SwapchainManager::existsMaxNumberOfSupportedImages(const VkSurfaceCapabilitiesKHR& capabilities) 
{
	return (capabilities.maxImageCount != 0);
}
