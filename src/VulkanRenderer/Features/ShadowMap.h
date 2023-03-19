#pragma once 

#include <memory>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <unordered_map>

#include "VulkanRenderer/Descriptor/Types/UBO/UBO.h"
#include "VulkanRenderer/Descriptor/Types/Sampler/Sampler.h"
#include "VulkanRenderer/Descriptor/DescriptorSets.h"
#include "VulkanRenderer/Descriptor/Types/DescriptorTypes.h"
#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/Pipeline/Graphics.h"
#include "VulkanRenderer/Model/Mesh.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"

#include "VulkanRenderer/Texture/Texture.h"

template<typename T>
class ShadowMap
{
public:

	struct ShadowModelInfo
	{
		DescriptorSets			modelDescriptorSets;
		std::shared_ptr<UBO>	modelUBO;
	};


	ShadowMap(
		const VkExtent2D& extent,
		const uint32_t imagesCount,
		const VkFormat& format,
		const uint32_t& uboCount,
		const std::vector<Mesh<T>>* meshes,
		const std::vector<size_t>& modelIndices
	);

	~ShadowMap();
	void destroy();

	void updateUBO(
		const glm::mat4 moodelM,
		const glm::fvec4 directionalLightStartPos,
		const glm::fvec4 directionalLightEndPos,
		const float aspect,
		const float zNear,
		const float zFar,
		const uint32_t& currentFrame,
		size_t  index
	);

	void bindData(
		const std::vector<Mesh<T>>* meshes,
		const size_t index,
		const VkCommandBuffer& commandBuffer,
		const uint32_t currentFrame
	);

	void createCommandPool(const VkCommandPoolCreateFlags& flags, const uint32_t& graphicsFamilyIndex);

	void allocCommandBuffers(const uint32_t& commandBuffersCount);

	const std::shared_ptr<TextureBase> get() const;
	const VkImageView& getShadowMapView() const;
	const VkSampler& getSampler() const;
	const glm::mat4& getLightSpace() const;
	const VkDescriptorSet& getDescriptorSet(const size_t index, const uint32_t currentFrame) const;
	const VkFramebuffer& getFramebuffer(const uint32_t imageIndex) const;
	const VkCommandBuffer& getCommandBuffer(const uint32_t index) const;

	const std::shared_ptr<CommandPool>& getCommandPool() const;
	const Graphics& getGraphicsPipeline() const;
	const RenderPass& getRenderPass() const;

private:

	void createUBO( const uint32_t& uboCount);

	void createDescriptorPool();
	void createDescriptorSets();
	void createGraphicsPipeline(const VkExtent2D& extent);
	void createRenderPass(const VkFormat& depthBufferFormat);
	void createFramebuffer(const uint32_t& imagesCount);


	uint32_t                         m_width;
	uint32_t                         m_height;

	std::shared_ptr<TextureBase>	 m_texture;

	RenderPass                       m_renderPass;

	DescriptorPool                   m_descriptorPool;
	//DescriptorSets                   m_descriptorSets;

	std::shared_ptr<CommandPool>     m_commandPool;

	std::vector<VkFramebuffer>       m_framebuffers;

	Graphics                         m_graphicsPipeline;

	DescriptorTypes::UniformBufferObject::ShadowMap m_basicInfo;

	mutable std::unordered_map<size_t, ShadowModelInfo>	m_shadowModelInfo;
	const std::vector<size_t>			m_modelIndices;

};