#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer/Window/WindowManager.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyHandles.h"
#include "VulkanRenderer/Swapchain/SwapchainManager.h"
#include "VulkanRenderer/GraphicsPipeline/GraphicsPipelineManager.h"
#include "VulkanRenderer/RenderPass/RenderPassManager.h"
#include "VulkanRenderer/Commands/CommandPool.h"
#include "VulkanRenderer/Device/Device.h"

class App
{
public:

	void run();

private:

	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();
	void drawFrame();

	void createVkInstance();

	void createSyncObjects();
	void destroySyncObjects();

	WindowManager            m_windowM;
	VkInstance               m_vkInstance;
	Device                   m_device;
	QueueFamilyIndices       m_qfIndices;
	QueueFamilyHandles       m_qfHandles;
	SwapchainManager         m_swapchainM;
	RenderPassManager        m_renderPassM;
	GraphicsPipelineManager  m_graphicsPipelineM;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	std::vector<CommandPool> m_commandPools;

	// Sync objects
	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkFence m_inFlightFence;
};