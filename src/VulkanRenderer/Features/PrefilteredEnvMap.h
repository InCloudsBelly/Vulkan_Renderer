#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/Pipeline/Graphics.h"
#include "VulkanRenderer/Texture/Texture.h"

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

    const std::shared_ptr<TextureBase> get() const;

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

    std::shared_ptr<TextureBase>     m_targetImage;
    std::shared_ptr <TextureBase>    m_offscreenImage;

    RenderPass                       m_renderPass;

    VkDescriptorSet                  m_descriptorSet;

    VkFramebuffer                    m_framebuffer;
    Graphics                         m_graphicsPipeline;

    PushBlockPrefilterEnv            m_pushBlock;
};