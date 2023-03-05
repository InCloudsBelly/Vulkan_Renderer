#pragma once

#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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


class Renderer
{
public:

	void run();

	

	void addObjectPBR(const std::string& name, const  const std::string& modelFileName,
		const glm::fvec3& pos = glm::fvec4(0.0f),
		const glm::fvec3& rot = glm::fvec3(0.0f),
		const glm::fvec3& size = glm::fvec3(1.0f)
	);

	void addSkybox(const std::string& name, const std::string& textureFolderName);

	void addDirectionalLight(const std::string& name, const std::string& modelFileName,
		const glm::fvec3& color,
		const glm::fvec3& pos,
		const glm::fvec3& endPos,
		const glm::fvec3& size
	);
	void addSpotLight(
		const std::string& name,
		const std::string& modelFileName,
		const glm::fvec3& color,
		const glm::fvec3& pos,
		const glm::fvec3& endPos,
		const glm::fvec3& rot,
		const glm::fvec3& size,
		const float attenuation,
		const float radius
	);

	void addPointLight(
		const std::string& name,
		const std::string& modelFileName,
		const glm::fvec3& color,
		const glm::fvec3& pos,
		const glm::fvec3& size,
		const float attenuation,
		const float radius
	);

private:
	void createPipelines();
	void createCommandPools();

	void uploadModels();
	void loadModel(const size_t startI, const size_t chunckSize);
	void loadModels();
	void initWindow();
	void initComputations();
	void handleInput();
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void initVulkan();
	void mainLoop();
	void cleanup();

	void createShadowMapRenderPass();
	void createSceneRenderPass();

	void doComputations();
	void loadBRDFlut();
	void configureUserInputs();

	void recordCommandBuffer(
		const VkFramebuffer& framebuffer,
		const RenderPass& renderPass,
		const VkExtent2D& extent,
		const std::vector<Graphics>& graphicsPipelines,
		const uint32_t currentFrame,
		const VkCommandBuffer& commandBuffer,
		const std::vector<VkClearValue>& clearValues,
		const std::shared_ptr<CommandPool>& commandPool
	);

	void drawFrame(uint8_t& currentFrame);

	void createSyncObjects();
	void destroySyncObjects();

	std::shared_ptr<Window>             m_window;
	std::unique_ptr<GUI>                m_GUI;
	std::shared_ptr<Camera>             m_camera;

	std::unique_ptr<VKinstance>         m_vkInstance;
	std::unique_ptr<Device>             m_device;
	VkDebugUtilsMessengerEXT            m_debugMessenger;
	QueueFamilyIndices                  m_qfIndices;
	QueueFamilyHandles                  m_qfHandles;

	std::shared_ptr<Swapchain>          m_swapchain;
	RenderPass                          m_renderPass;
	RenderPass                          m_renderPassShadowMap;

	std::vector<VkSemaphore>            m_imageAvailableSemaphores;
	std::vector<VkSemaphore>            m_renderFinishedSemaphores;
	std::vector<VkFence>                m_inFlightFences;

	std::vector<ModelInfo>              m_modelsToLoadInfo;
	std::shared_ptr<Skybox>             m_skybox;
	std::vector<std::shared_ptr<Model>> m_models;
	std::vector<size_t>                 m_objectModelIndices;
	std::vector<size_t>                 m_lightModelIndices;
	std::vector<size_t>                 m_skyboxModelIndex;
	std::optional<size_t>               m_directionalLightIndex;

	
	// TODO: delete this
	size_t								m_mainModelIndex;

	// Pipelines
	Graphics                            m_graphicsPipelinePBR;
	Graphics                            m_graphicsPipelineSkybox;
	Graphics                            m_graphicsPipelineLight;
	Graphics                            m_graphicsPipelineShadowMap;

	//Computations
	Computation                         m_BRDFcomp;

	//
	std::shared_ptr<Texture>            m_BRDFlut;

	// Command Pool for main drawing commands.
	std::shared_ptr<CommandPool>        m_commandPoolGraphics;
	std::shared_ptr<CommandPool>        m_commandPoolCompute;

	DescriptorPool                      m_descriptorPoolGraphics;
	DescriptorPool                      m_descriptorPoolComputations;


	std::vector<VkClearValue>			m_clearValues;
	std::vector<VkClearValue>			m_clearValuesShadowMap;
	bool								m_isMouseInMotion;

	//---------------------------Features--------------------------------------
	DepthBuffer							m_depthBuffer;
	MSAA								m_msaa;
	std::shared_ptr<ShadowMap<Attributes::PBR::Vertex>> m_shadowMap;
};