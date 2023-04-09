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
#include "VulkanRenderer/Features/DepthBuffer.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/Device/Device.h"
#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/Features/ShadowMap.h"
#include "VulkanRenderer/VKinstance/VKinstance.h"
#include "VulkanRenderer/Scene/ForwardPBR.h"
#include "VulkanRenderer/Guid_Allocator.h"

#include "VulkanRenderer/RenderResource.h"
#include "VulkanRenderer/InputManager.h"

#include "VulkanRenderer/Scene/ForwardPBR.h"
#include "VulkanRenderer/Scene/DeferredRenderPass.h"

#ifndef getRendererPointer
#define getRendererPointer() g_RendererSingleton
#endif

#ifndef getRenderResource
#define getRenderResource() g_RenderResource
#endif

#ifndef getInputManager
#define getInputManager() g_InputManager
#endif

class Renderer;

extern RenderResource* g_RenderResource;
extern InputManager* g_InputManager;
extern Renderer* g_RendererSingleton ;

class Renderer
{
public:

	void run();

	void addObjectPBR(const std::string& name, const std::string& folderName,const std::string& fileName,const glm::fvec3& pos = glm::fvec3(0.0f),const glm::fvec3& rot = glm::fvec3(0.0f),const glm::fvec3& size = glm::fvec3(1.0f));

	void addSkybox(const std::string& fileName, const std::string& textureFolderName);

	void addDirectionalLight(const std::string& name, const std::string& folderName,const std::string& fileName,const glm::fvec3& color,const glm::fvec3& pos,const glm::fvec3& endPos,const glm::fvec3& size);
	void addSpotLight(const std::string& name,const std::string& folderName,const std::string& fileName,const glm::fvec3& color,const glm::fvec3& pos,const glm::fvec3& endPos,const glm::fvec3& rot,const glm::fvec3& size);
	void addPointLight(const std::string& name,const std::string& folderName,const std::string& fileName,const glm::fvec3& color,const glm::fvec3& pos,const glm::fvec3& size);

	const VkInstance& getVKinstance()const 					{ return m_vkInstance->get(); };
	const std::shared_ptr<Window>	getWindow()const		{ return m_window; }
	const std::shared_ptr<Swapchain> getSwapchain() const	{ return m_swapchain; }
	const MSAA* getMSAA() const								{ return &m_msaa; }
	const DepthBuffer* getDepthBuffer() const				{ return &m_depthBuffer; }
	virtual VkDevice getDevice()							{ return m_device->getLogicalDevice();};
	virtual VkPhysicalDevice getPhysicalDevice()			{ return m_device->getPhysicalDevice();};
	virtual VmaAllocator& getVmaAllocator()					{ return m_vmaAllocator;};
	virtual VkQueue& getGraphicsQueue()						{ return m_qfHandles.graphicsQueue;};
	virtual QueueFamilyIndices& getQueueFamilyIndices()		{ return m_qfIndices; }
	virtual VkCommandPool getCommandPool()					{ return m_commandPoolForGraphics;};
	virtual VkDescriptorPool& getDescriptorPool()			{ return m_descriptorPool; }
	
	
	const double& getMicroSecondPerFrame() const			{ return m_mpf; }
	const std::string& getDeviceName() const				{ return m_device->getDeviceName(); }
	const uint32_t& getApiVersion()const					{ return m_device->getApiVersion(); }

	VkCommandBuffer& getGraphicsCommandBuffer(uint32_t index) { return m_commandBuffersForGraphics[index]; }
	VkCommandBuffer& getComputeCommandBuffer(uint32_t index) { return m_commandBuffersForCompute[index]; }

	SwapChainDesc getSwapchainInfo();
	DepthImageDesc getDepthImageInfo();
	MSAADesc getMSAAInfo();

private:
	void doComputations();
	void calculateFrames(double& lastTime, int& framesCounter);

	void initVulkan();
	void mainLoop();
	void cleanup();

	void drawFrame(uint8_t& currentFrame);

	void createSyncObjects();
	void destroySyncObjects();

	VkResult createVMAAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& allocator);
	
	std::shared_ptr<Window>             m_window;

	std::unique_ptr<VKinstance>         m_vkInstance;
	std::unique_ptr<Device>             m_device;
	VkDebugUtilsMessengerEXT            m_debugMessenger;
	QueueFamilyIndices                  m_qfIndices;
	QueueFamilyHandles                  m_qfHandles;

	VmaAllocator						m_vmaAllocator;

	std::shared_ptr<Swapchain>          m_swapchain;
	
	std::unique_ptr<DeferredRenderPass>			m_scene;
	//std::unique_ptr<ForwardPBRPass>			m_scene;

	//std::vector<VkCommandPool>			m_commandPools = {}; ///<Array of command pools so that each thread in the thread pool has its own pool
	//std::vector<VkCommandBuffer>		m_commandBuffers = {}; ///<the main command buffers for recording draw commands

	std::vector<VkSemaphore>            m_imageAvailableSemaphores;
	std::vector<VkSemaphore>            m_renderFinishedSemaphores;
	std::vector<VkFence>                m_inFlightFences;

	std::vector<ModelInfo>              m_modelsToLoadInfo;

	// Command Pool for main drawing commands.
	VkCommandPool						m_commandPoolForGraphics;
	VkCommandPool						m_commandPoolForCompute;

	std::vector<VkCommandBuffer>		m_commandBuffersForGraphics;
	std::vector<VkCommandBuffer>		m_commandBuffersForCompute;

	VkDescriptorPool                    m_descriptorPool;


	bool								m_isMouseInMotion;


	// milliseconds per frame
	double												m_mpf;
	//---------------------------Features--------------------------------------
	DepthBuffer											m_depthBuffer;
	MSAA												m_msaa;
};