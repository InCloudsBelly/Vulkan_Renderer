#pragma once 

#include <memory>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <unordered_map>

#include "VulkanRenderer/Descriptor/DescriptorTypes.h"
#include "VulkanRenderer/Pipeline/Graphics.h"
#include "VulkanRenderer/Model/Mesh.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"

#include "VulkanRenderer/Texture/Texture.h"

template<typename T>
class ShadowMap
{
public:

	ShadowMap(
		const VkExtent2D& extent,
		const uint32_t imagesCount,
		const VkFormat& format,
		const uint32_t& uboCount
	);

	~ShadowMap();
	void destroy();

	void updateUBO();

	void bindData(
		const VkCommandBuffer& commandBuffer,
		const uint32_t currentFrame
	);

	const std::shared_ptr<TextureBase> get() const;
	const VkImageView& getShadowMapView() const;
	const VkSampler& getSampler() const;
	const glm::mat4& getLightSpace() const;
	//const VkDescriptorSet& getDescriptorSet(const size_t index, const uint32_t currentFrame) const;
	const VkFramebuffer& getFramebuffer(const uint32_t imageIndex) const;
	const VkCommandBuffer& getCommandBuffer(const uint32_t index) const;

	const VkCommandPool& getCommandPool() const;
	const Graphics& getGraphicsPipeline() const;
	const RenderPass& getRenderPass() const;

private:
	void createGraphicsPipeline(const VkExtent2D& extent);
	void createRenderPass(const VkFormat& depthBufferFormat);
	void createFramebuffer(const uint32_t& imagesCount);


	uint32_t                         m_width;
	uint32_t                         m_height;

	std::shared_ptr<TextureBase>	 m_texture;

	RenderPass                       m_renderPass;

	VkDescriptorPool				 m_descriptorPool;


	VkCommandPool					 m_commandPool;
	std::vector<VkCommandBuffer>	 m_commandBuffers;

	std::vector<VkFramebuffer>       m_framebuffers;

	Graphics                         m_graphicsPipeline;

	DescriptorTypes::UniformBufferObject::ShadowMap m_basicInfo;

	std::vector<VkDescriptorSet>		m_modelDescriptorSets;

	std::vector<VkBuffer>				m_ubos;
	std::vector<VmaAllocation>			m_uboAllocations;
};