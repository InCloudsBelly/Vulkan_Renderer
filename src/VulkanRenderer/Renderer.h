#pragma once

#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer/Window/WindowManager.h"
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

	void initImgui();
	void imguiRender(const uint8_t currentFrame, const uint8_t imageIndex);

private:

	// Modify this
	void updateUniformBuffer1(
		const VkDevice& logicalDevice,
		const uint8_t currentFrame,
		const VkExtent2D extent,
		std::vector<VkDeviceMemory>& uniformBufferMemories
	);
	// Modify this
	void updateUniformBuffer2(
		const VkDevice& logicalDevice,
		const uint8_t currentFrame,
		const VkExtent2D extent,
		std::vector<VkDeviceMemory>& uniformBufferMemories
	);


	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

	void recordCommandBuffer(
		const VkFramebuffer& framebuffer,
		const VkRenderPass& renderPass,
		const VkExtent2D& extent,
		const VkPipeline& graphicsPipeline,
		const VkPipelineLayout& pipelineLayout,
		const uint32_t currentFrame,
		VkCommandBuffer& commandBuffer,
		CommandPool& commandPool
	);

	void drawFrame(uint8_t& currentFrame);

	void createVkInstance();

	void createSyncObjects();
	void destroySyncObjects();

	WindowManager				m_windowM;
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
	VkDescriptorSetLayout		m_descriptorSetLayout;


	// Imgui
	std::vector<VkFramebuffer>  m_framebuffersImgui;
	CommandPool					m_commandPoolImgui;
	std::vector<VkCommandBuffer> m_commandBuffersImgui;
	DescriptorPool				m_descriptorPoolImgui;

	VkRenderPass				m_renderPassImgui;

	// NUMBER OF VK_ATTACHMENT_LOAD_OP_CLEAR == CLEAR_VALUES
	std::vector<VkClearValue>	clearValues;
};