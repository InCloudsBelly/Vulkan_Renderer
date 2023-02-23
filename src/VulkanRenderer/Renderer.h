#pragma once

#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer/Window/Window.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyIndices.h"
#include "VulkanRenderer/QueueFamily/QueueFamilyHandles.h"
#include "VulkanRenderer/Swapchain/Swapchain.h"
#include "VulkanRenderer/GraphicsPipeline/GraphicsPipelineManager.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/Commands/CommandPool.h"
#include "VulkanRenderer/Device/Device.h"
#include "VulkanRenderer/Buffers/BufferManager.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Textures/Texture.h"
#include "VulkanRenderer/Model/Model.h"

class Renderer
{
public:

	void run();
	void addModel(const std::string& meshFile, const std::string& textureFile);

private:

	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();
	void drawFrame(uint8_t& currentFrame);

	void createVkInstance();

	void createSyncObjects();
	void destroySyncObjects();

	Window						m_window;
	VkInstance					m_vkInstance;
	Device						m_device;
	QueueFamilyIndices			m_qfIndices;
	QueueFamilyHandles			m_qfHandles;
	Swapchain					m_swapchain;
	RenderPass					m_renderPass;
	GraphicsPipelineManager		m_graphicsPipelineM;
	VkDebugUtilsMessengerEXT	m_debugMessenger;
	std::vector<CommandPool>	m_commandPools;

	DescriptorPool				m_descriptorPool;
	DepthBuffer					m_depthBuffer;
	// Future improv.
	//CommandPool				m_commandPoolMemoryAlloc;

	// Sync objects(for each frame)
	std::vector<VkSemaphore>	m_imageAvailableSemaphores;
	std::vector<VkSemaphore>	m_renderFinishedSemaphores;
	std::vector<VkFence>		m_inFlightFences;

	// Models
	std::vector<std::unique_ptr<Model>> m_models;
};