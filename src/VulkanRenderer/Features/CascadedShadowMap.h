//#pragma once 
//
//#include <memory>
//
//#include <vulkan/vulkan.h>
//#include <glm/glm.hpp>
//#include <unordered_map>
//
//#include "VulkanRenderer/Descriptor/DescriptorTypes.h"
//
//#include "VulkanRenderer/RenderPass/RenderPass.h"
//
//
//
//
//class CascadedShadowMap
//{
//public:
//
//	CascadedShadowMap(
//		const VkExtent2D& extent,
//		const uint32_t imagesCount,
//		const VkFormat& format,
//		const float& lambda,
//		const float& near_offser,
//		const uint32_t& split_count
//	);
//
//	~CascadedShadowMap();
//	void destroy();
//
//	void updateUBO();
//
//	void draw(uint32_t imageIndex, uint32_t frameIndex);
//
//	const std::shared_ptr<TextureBase> get() const;
//	const VkImageView& getCascadedShadowMapView() const;
//	const VkSampler& getSampler() const;
//	const glm::mat4& getLightSpace() const;
//	//const VkDescriptorSet& getDescriptorSet(const uint32_t index, const uint32_t currentFrame) const;
//	const VkFramebuffer& getFramebuffer(const uint32_t imageIndex) const;
//	const VkCommandBuffer& getCommandBuffer(const uint32_t index) const;
//
//	const VkCommandPool& getCommandPool() const;
//	const RenderPass& getRenderPass() const;
//
//private:
//	void createGraphicsPipeline(const VkExtent2D& extent);
//	void createRenderPass(const VkFormat& depthBufferFormat);
//	void createFramebuffer(const uint32_t& imagesCount);
//	void createUBOs();
//	void createDescriptorSets();
//
//	float							 m_lambda;
//	float							 m_near_offset;
//	uint32_t						 m_split_count;
//
//	uint32_t                         m_width;
//	uint32_t                         m_height;
//
//	std::vector<VkClearValue>		 m_clearValuesCascadedShadowMap;
//
//	std::shared_ptr<TextureBase>	 m_texture;
//
//	RenderPass                       m_renderPass;
//
//	VkDescriptorPool				 m_descriptorPool;
//
//
//	VkCommandPool					 m_commandPool;
//	std::vector<VkCommandBuffer>	 m_commandBuffers;
//
//	std::vector<VkFramebuffer>       m_framebuffers;
//
//	VkPipeline                       m_pipeline;
//	VkDescriptorSetLayout            m_descriptorSetLayout;
//	VkPipelineLayout                 m_pipelineLayout;
//
//	DescriptorTypes::UniformBufferObject::ShadowMap m_basicInfo;
//
//	std::unordered_map<uint32_t, VkBuffer>			m_ubosMap;
//	std::unordered_map<uint32_t, VmaAllocation>		m_uboAllocationsMap;
//	std::unordered_map<uint32_t, VkDescriptorSet>	m_descriptorSetsMap;
//};