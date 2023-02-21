#pragma once

#include <vector>
#include <optional>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Window/WindowManager.h"

struct SwapchainSupportedProperties
{
	// Basic surface capabilities:
	// - Min/max number of images in swap chain.
	// - Min/max width and height of images.
	VkSurfaceCapabilitiesKHR capabilities;
	// Surface formats:
	//    - Pixel format ->
	//    - Color space  -> 
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	// Presentation modes: 
	//    The logic of how the images will be diplayed on the screen.
	std::vector<VkPresentModeKHR> presentModes;
};

class SwapchainManager
{
public:
	SwapchainManager();
	~SwapchainManager() {}

	void createSwapchain(
		const VkPhysicalDevice& physicalDevice,
		const VkDevice& logicalDevice,
		const WindowManager& windowM
	);

	void createImageViews(const VkDevice& logicalDevice);
	void createFramebuffers(const VkDevice& logicalDevice, const VkRenderPass& renderPass);

	void destroyFramebuffers(const VkDevice& logicalDevice);
	void destroySwapchain(const VkDevice& logicalDevice);
	void destroyImageViews(const VkDevice& logicalDevice);

	SwapchainSupportedProperties getSupportedProperties(
		const VkPhysicalDevice& physicalDevice,
		const VkSurfaceKHR& surface);

	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseBestPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D chooseBestExtent(const VkSurfaceCapabilitiesKHR& capabilities, const WindowManager& windowM);

	void chooseBestSettings(
		const VkPhysicalDevice& physicalDevice,
		const WindowManager& windowM,
		VkSurfaceFormatKHR& surfaceFormat,
		VkPresentModeKHR& presentMode,
		VkExtent2D& extent
	);

	const VkExtent2D& getExtent() const;
	const VkFormat& getImageFormat() const;

	// Used in isDeviceSuitable function.
	bool isSwapchainAdequated(
		const VkPhysicalDevice& physicalDevice,
		const VkSurfaceKHR& surface
	);

	bool existsMaxNumberOfSupportedImages(
		const VkSurfaceCapabilitiesKHR& capabilities
	);

private:
	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_images;

	// Describes how to access the images and which part of the images to
	// access.
	std::vector<VkImageView> m_imageViews;

	std::optional<SwapchainSupportedProperties> m_supportedProperties;
	VkFormat m_imageFormat;
	VkExtent2D m_extent;

	std::vector<VkFramebuffer> m_framebuffers;
};