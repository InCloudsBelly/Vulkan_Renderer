#pragma once

#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <VMA/vk_mem_alloc.h>

#include "VulkanRenderer/Window/Window.h"
#include "VulkanRenderer/GUI/GUI.h"
#include "VulkanRenderer/Queue/QueueFamilyIndices.h"
#include "VulkanRenderer/Queue/QueueFamilyHandles.h"
#include "VulkanRenderer/Swapchain/Swapchain.h"
#include "VulkanRenderer/Computation/Computation.h"
#include "VulkanRenderer/Pipeline/Graphics.h"
#include "VulkanRenderer/Pipeline/Compute.h"
#include "VulkanRenderer/Features/DepthBuffer.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/Device/Device.h"
#include "VulkanRenderer/Descriptor/DescriptorPool.h"
#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Model/ModelInfo.h"
#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/Model/Types/Skybox.h"
#include "VulkanRenderer/Model/Types/Light.h"
#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/Camera/Types/Arcball.h"
#include "VulkanRenderer/Features/ShadowMap.h"
#include "VulkanRenderer/VKinstance/VKinstance.h"
#include "VulkanRenderer/Scene/Scene.h"


#ifndef getRendererPointer
#define getRendererPointer() g_RendererSingleton
#endif

class Renderer;

extern Renderer* g_RendererSingleton ;

class Renderer
{
public:

	void run();

	

	void addObjectPBR(const std::string& name, 
		const std::string& folderName,
		const std::string& fileName,
		const glm::fvec3& pos = glm::fvec3(0.0f),
		const glm::fvec3& rot = glm::fvec3(0.0f),
		const glm::fvec3& size = glm::fvec3(1.0f)
	);

	void addSkybox(const std::string& fileName, const std::string& textureFolderName);

	void addDirectionalLight(const std::string& name, 
		const std::string& folderName,
		const std::string& fileName,
		const glm::fvec3& color,
		const glm::fvec3& pos,
		const glm::fvec3& endPos,
		const glm::fvec3& size
	);
	void addSpotLight(
		const std::string& name,
		const std::string& folderName,
		const std::string& fileName,
		const glm::fvec3& color,
		const glm::fvec3& pos,
		const glm::fvec3& endPos,
		const glm::fvec3& rot,
		const glm::fvec3& size
	);

	void addPointLight(
		const std::string& name,
		const std::string& folderName,
		const std::string& fileName,
		const glm::fvec3& color,
		const glm::fvec3& pos,
		const glm::fvec3& size
	);

	virtual VkDevice getDevice()
	{
		return m_device->getLogicalDevice();
	};

	virtual VkPhysicalDevice getPhysicalDevice()
	{
		return m_device->getPhysicalDevice();
	};

	virtual VmaAllocator& getVmaAllocator()
	{
		return m_vmaAllocator;
	};

	virtual VkQueue& getGraphicsQueue()
	{
		return m_qfHandles.graphicsQueue;
	};

	///returns the thread command pool
	virtual VkCommandPool getCommandPool()
	{
		return m_commandPoolForGraphics->get();
	};

private:
	void createCommandPools();
	void initWindow();
	void doComputations();
	void handleInput();
	void calculateFrames(double& lastTime, int& framesCounter);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void initVulkan();
	void mainLoop();
	void cleanup();

	void configureUserInputs();

	void recordCommandBuffer(
		const VkFramebuffer& framebuffer,
		const RenderPass& renderPass,
		const VkExtent2D& extent,
		const std::vector<const Graphics*>& graphicsPipelines,
		const uint32_t currentFrame,
		const VkCommandBuffer& commandBuffer,
		const std::vector<VkClearValue>& clearValues,
		const std::shared_ptr<CommandPool>& commandPool
	);

	void drawFrame(uint8_t& currentFrame);

	void createSyncObjects();
	void destroySyncObjects();

	VkResult vhMemCreateVMAAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& allocator);
	
	std::shared_ptr<Window>             m_window;
	std::unique_ptr<GUI>                m_GUI;
	std::shared_ptr<Camera>             m_camera;

	std::unique_ptr<VKinstance>         m_vkInstance;
	std::unique_ptr<Device>             m_device;
	VkDebugUtilsMessengerEXT            m_debugMessenger;
	QueueFamilyIndices                  m_qfIndices;
	QueueFamilyHandles                  m_qfHandles;

	VmaAllocator						m_vmaAllocator;

	std::shared_ptr<Swapchain>          m_swapchain;
	
	Scene                               m_scene;

	std::vector<VkSemaphore>            m_imageAvailableSemaphores;
	std::vector<VkSemaphore>            m_renderFinishedSemaphores;
	std::vector<VkFence>                m_inFlightFences;

	std::vector<ModelInfo>              m_modelsToLoadInfo;

	// Command Pool for main drawing commands.
	std::shared_ptr<CommandPool>        m_commandPoolForGraphics;
	std::shared_ptr<CommandPool>        m_commandPoolForCompute;

	DescriptorPool                      m_descriptorPoolForGraphics;
	DescriptorPool                      m_descriptorPoolForComputations;


	std::vector<VkClearValue>			m_clearValues;
	std::vector<VkClearValue>			m_clearValuesShadowMap;
	bool								m_isMouseInMotion;

	// milliseconds per frame
	double								m_mpf;
	//---------------------------Features--------------------------------------
	DepthBuffer											m_depthBuffer;
	MSAA												m_msaa;
	std::shared_ptr<ShadowMap<Attributes::PBR::Vertex>> m_shadowMap;
};