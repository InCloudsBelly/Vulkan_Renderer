#pragma once

#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Window/Window.h"
#include "VulkanRenderer/Features/MSAA.h"
#include "VulkanRenderer/Features/DepthBuffer.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"

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
		const std::shared_ptr<Window>& window,
		const SwapchainSupportedProperties& supportedProperties
	);
	~Swapchain();

	void presentImage(const uint32_t imageIndex, const std::vector<VkSemaphore> signalSemaphores, const VkQueue& presentQueue);

	void destroy();

	const uint32_t getNextImageIndex(const VkSemaphore& semaphore) const;

	const VkExtent2D& getExtent() const;
	const VkFormat& getImageFormat() const;
	VkViewport& getViewport()  { return m_viewport; }
	VkRect2D& getScissor() { return m_scissor; }
	std::vector<VkImageView*> getImageViews() { return m_imageViews; }

	const VkSwapchainKHR& get() const;
	const uint32_t getImageCount() const;
	const uint32_t getMinImageCount() const;

	const VkImageView& getImageView(const uint32_t index) const;

private:
	void chooseBestSettings(const std::shared_ptr<Window>& window,const SwapchainSupportedProperties& supportedProperties,
			VkSurfaceFormatKHR& surfaceFormat,VkPresentModeKHR& presentMode,VkExtent2D& extent);

	void createAllImageViews();

	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseBestPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D chooseBestExtent(const VkSurfaceCapabilitiesKHR& capabilities, const std::shared_ptr<Window>& window);

	const bool existsMaxNumberOfSupportedImages(const VkSurfaceCapabilitiesKHR& capabilities)const;


private:
	VkDevice                   m_logicalDevice;

	VkSwapchainKHR             m_swapchain;
	VkFormat                   m_imageFormat;
	VkExtent2D                 m_extent;
	VkViewport				   m_viewport;
	VkRect2D				   m_scissor;
	std::vector<VkImage>       m_images;
	std::vector<VkImageView*>  m_imageViews;

	// Used for the creation of the Imgui instance.
	uint32_t                   m_minImageCount;
};