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
#include "VulkanRenderer/Buffers/BufferManager.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"

class Renderer
{
public:

	void run();

private:

	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();
	void drawFrame(uint8_t& currentFrame);

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

	DescriptorPool           m_descriptorPool;
	// Future improv.
	//CommandPool              m_commandPoolMemoryAlloc;

	// Sync objects(for each frame)
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence>     m_inFlightFences;

	// Buffers with their memories
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_memory1;

	VkBuffer m_indexBuffer;
	VkDeviceMemory m_memory2;
};