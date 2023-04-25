#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanRenderer/RenderPass/RenderPass.h"


#include "VulkanRenderer/Image/Image.h"

struct PushBlockPrefilterEnv
{
	glm::mat4 mvp;
	float roughness;
	int samplesCount = 32;
};


class PrefilteredEnvMap
{
public:

    PrefilteredEnvMap(const uint32_t dim);
    ~PrefilteredEnvMap();

    void destroy();

    const Texture& get() const;

private:

    void createPipeline();
    void createOffscreenFramebuffer();
    void createRenderPass();
    void createTargetImage();
    void createDescriptorSet();
    void copyRegionOfImage(
        float face,
        float mipLevel,
        float viewportDim,
        const VkCommandBuffer& commandBuffer
    );
    void recordCommandBuffer();


    uint32_t                         m_dim;
    VkFormat                         m_format;
    uint32_t                         m_mipLevels;

    Image*                           m_offscreenImage;
    Texture                          m_targetTex;

    RenderPass                       m_renderPass;

    VkDescriptorSet                  m_descriptorSet;

    VkFramebuffer                    m_framebuffer;

    VkPipeline                       m_pipeline;
    VkDescriptorSetLayout            m_descriptorSetLayout;
    VkPipelineLayout                 m_pipelineLayout;

    PushBlockPrefilterEnv            m_pushBlock;
};