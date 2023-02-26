#pragma once

#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer/Window/Window.h"
#include "VulkanRenderer/GUI/GUI.h"
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
	void addNormalModel(const std::string& name, const std::string& meshFile, const std::string& textureFile = "default.jpg");
	void addLightModel(const std::string& name, const std::string& meshFile, const std::string& textureFile = "default.jpg");


private:
	void addModel(const std::string& name, const std::string& meshFile, const std::string& textureFile);

	// Modify this
	void updateUniformBuffer(
		const VkDevice& logicalDevice,
		const uint8_t currentFrame,
		const VkExtent2D extent,
		Model& model
	);

	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

	void createRenderPass();

	void recordCommandBuffer(
		const VkFramebuffer& framebuffer,
		const RenderPass& renderPass,
		const VkExtent2D& extent,
		const VkPipeline& graphicsPipeline,
		const VkPipelineLayout& pipelineLayout,
		const uint32_t currentFrame,
		const VkCommandBuffer& commandBuffer,
		CommandPool& commandPool
	);

	void drawFrame(uint8_t& currentFrame);

	void createVkInstance();

	void createSyncObjects();
	void destroySyncObjects();

	Window						m_window;
	std::unique_ptr<GUI>		m_GUI;
	VkInstance					m_vkInstance;
	Device						m_device;
	QueueFamilyIndices			m_qfIndices;
	QueueFamilyHandles			m_qfHandles;
	std::unique_ptr<Swapchain>	m_swapchain;
	RenderPass					m_renderPass;
	VkDebugUtilsMessengerEXT	m_debugMessenger;
	GraphicsPipelineManager		m_graphicsPipelineM;
	// Command buffer for main drawing commands.
	CommandPool					m_commandPool;

	DepthBuffer					m_depthBuffer;

	// Sync objects(for each frame)
	std::vector<VkSemaphore>	m_imageAvailableSemaphores;
	std::vector<VkSemaphore>	m_renderFinishedSemaphores;
	std::vector<VkFence>		m_inFlightFences;

	// Models
	std::vector<std::shared_ptr<Model>> m_allModels;
	// The light models will be saved also in m_allModels but we will
	// keep track of them in m_lightModels.
	std::vector<size_t> m_lightModelIndices;
	// Models that interact with the light.
	std::vector<size_t> m_normalModelIndices;

	DescriptorPool				m_descriptorPool;
	VkDescriptorSetLayout		m_descriptorSetLayout;

	std::vector<VkClearValue>	m_clearValues;
	glm::fvec3					m_cameraPos;
};