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
#include "VulkanRenderer/Commands/CommandPool.h"
#include "VulkanRenderer/Device/Device.h"
#include "VulkanRenderer/BufferManager/BufferManager.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Textures/Texture.h"
#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/Model/Types/Skybox.h"
#include "VulkanRenderer/Model/Types/Light.h"
#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/Camera/Types/Arcball.h"
#include "VulkanRenderer/Features/ShadowMap.h"


struct ModelToLoadInfo
{
	ModelType   type;
	std::string name;
	std::string modelFileName;
	glm::fvec3  color;
	glm::fvec3  pos;
	glm::fvec3  rot;
	glm::fvec3  size;

	// For light models.
	LightType   lType;
	glm::fvec3  endPos;
	const float attenuation;
	const float radius;
};


class Renderer
{
public:

	void run();

	void loadModel(const size_t startI, const size_t chunckSize);
	void loadModels();

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

	void initWindow();
	void initComputations();
	void handleInput();
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void initVulkan();
	void mainLoop();
	void cleanup();

	void createShadowMapRenderPass();
	void createSceneRenderPass();

	void createDescriptorSetLayouts();
	void doComputations();

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

	template <typename T>
	void bindAllMeshesData(
		const std::shared_ptr<T>& model,
		const Graphics& graphicsPipeline,
		const VkCommandBuffer& commandBuffer,
		const uint32_t currentFrame
	);

	void drawFrame(uint8_t& currentFrame);

	void createVkInstance();

	void createSyncObjects();
	void destroySyncObjects();

	std::shared_ptr<Window>             m_window;
	std::unique_ptr<GUI>                m_GUI;
	std::shared_ptr<Camera>             m_camera;

	VkInstance                          m_vkInstance;
	Device                              m_device;
	VkDebugUtilsMessengerEXT            m_debugMessenger;
	QueueFamilyIndices                  m_qfIndices;
	QueueFamilyHandles                  m_qfHandles;

	std::unique_ptr<Swapchain>          m_swapchain;
	RenderPass                          m_renderPass;
	RenderPass                          m_renderPassShadowMap;

	std::vector<VkSemaphore>            m_imageAvailableSemaphores;
	std::vector<VkSemaphore>            m_renderFinishedSemaphores;
	std::vector<VkFence>                m_inFlightFences;

	std::vector<ModelToLoadInfo>        m_modelsToLoadInfo;
	std::vector<std::shared_ptr<Model>> m_models;
	std::vector<size_t>                 m_objectModelIndices;
	std::vector<size_t>                 m_lightModelIndices;
	std::vector<size_t>                 m_skyboxModelIndex;
	std::optional<size_t>               m_directionalLightIndex;
	std::shared_ptr<Skybox>             m_skybox;

	
	// TODO: delete this
	size_t								m_mainModelIndex;

	// Pipelines
	Graphics                            m_graphicsPipelinePBR;
	Graphics                            m_graphicsPipelineSkybox;
	Graphics                            m_graphicsPipelineLight;
	Graphics                            m_graphicsPipelineShadowMap;

	//Computations
	Computation                         m_BRDF;

	// Command Pool for main drawing commands.
	std::shared_ptr<CommandPool>        m_commandPoolGraphics;
	std::shared_ptr<CommandPool>        m_commandPoolCompute;

	DescriptorPool                      m_descriptorPoolGraphics;
	DescriptorPool                      m_descriptorPoolComputations;

	VkDescriptorSetLayout               m_descriptorSetLayoutNormalPBR;
	VkDescriptorSetLayout               m_descriptorSetLayoutLight;
	VkDescriptorSetLayout               m_descriptorSetLayoutSkybox;
	VkDescriptorSetLayout               m_descriptorSetLayoutShadowMap;

	std::vector<VkClearValue>			m_clearValues;
	std::vector<VkClearValue>			m_clearValuesShadowMap;
	bool								m_isMouseInMotion;

	//---------------------------Features--------------------------------------
	DepthBuffer							m_depthBuffer;
	MSAA								m_msaa;
	std::shared_ptr<ShadowMap>			m_shadowMap;
	std::shared_ptr<Texture>			m_irradianceMap;
};