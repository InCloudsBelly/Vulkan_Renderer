#pragma once

#include <vector>
#include <optional>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Window/Window.h"
#include "VulkanRenderer/GraphicsPipeline/RenderTarget.h"


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

class Swapchain
{
public:
	Swapchain();
	Swapchain(
		const VkPhysicalDevice& physicalDevice,
		const VkDevice& logicalDevice,
		const Window& window,
		const SwapchainSupportedProperties& supportedProperties
	);
	~Swapchain();

	void createAllImageViews(const VkDevice& logicalDevice);
	void createFramebuffers(const VkDevice& logicalDevice, const VkRenderPass& renderPass, const RenderTarget::DepthBuffer& depthBuffer, const RenderTarget::MSAA& msaa);

	void destroyFramebuffers(const VkDevice& logicalDevice);
	void destroySwapchain(const VkDevice& logicalDevice);
	void destroyImageViews(const VkDevice& logicalDevice);

	const VkExtent2D& getExtent() const;
	const VkFormat& getImageFormat() const;
	const VkFramebuffer& getFramebuffer(const uint32_t imageIndex) const;
	const VkSwapchainKHR& get() const;
	const uint32_t getImageCount() const;
	const uint32_t getMinImageCount() const;
	const VkImageView& getImageView(const uint32_t index) const;

private:
	void chooseBestSettings(const Window& window,const SwapchainSupportedProperties& supportedProperties,
			VkSurfaceFormatKHR& surfaceFormat,VkPresentModeKHR& presentMode,VkExtent2D& extent);

	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseBestPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D chooseBestExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Window& window);

	const bool existsMaxNumberOfSupportedImages(const VkSurfaceCapabilitiesKHR& capabilities)const;


private:
	VkSwapchainKHR					m_swapchain;
	std::vector<VkImage>			m_images;
	// Used for the creation of the Imgui instance.
	uint32_t						m_minImageCount;

	// Describes how to access the images and which part of the images to
	// access.
	std::vector<VkImageView>		m_imageViews;

	VkFormat						m_imageFormat;
	VkExtent2D						m_extent;

	std::vector<VkFramebuffer>		m_framebuffers;
};