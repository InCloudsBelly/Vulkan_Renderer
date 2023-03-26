#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/Pipeline/Graphics.h"
#include "VulkanRenderer/Texture/Texture.h"

struct PushBlockPrefilterEnv
{
	glm::mat4 mvp;
	float roughness;
	int samplesCount = 32;
};

template<typename T>
class PrefilteredEnvMap
{
public:

    PrefilteredEnvMap(
        const VkQueue& graphicsQueue,
        const VkCommandPool& commandPool,
        const uint32_t dim,
        const std::vector<Mesh<T>>& meshes,
        const std::shared_ptr<TextureBase>& envMap
    );
    ~PrefilteredEnvMap();

    void destroy();

    const std::shared_ptr<TextureBase> get() const;

private:

    void createPipeline();
    void createOffscreenFramebuffer(
        const VkQueue& graphicsQueue
    );
    void createRenderPass();
    void createTargetImage();
    void createDescriptorPool();
    void createDescriptorSet(const std::shared_ptr<TextureBase>& envMap);
    void copyRegionOfImage(
        float face,
        float mipLevel,
        float viewportDim,
        const VkCommandBuffer& commandBuffer
    );
    void recordCommandBuffer(
        const VkCommandPool& commandPool,
        const VkQueue& graphicsQueue,
        const std::vector<Mesh<T>>& meshes
    );


    uint32_t                         m_dim;
    VkFormat                         m_format;
    uint32_t                         m_mipLevels;

    std::shared_ptr<TextureBase>     m_targetImage;
    std::shared_ptr <TextureBase>    m_offscreenImage;

    RenderPass                       m_renderPass;

    VkDescriptorSet                  m_descriptorSet;
    VkDescriptorPool                 m_descriptorPool;

    VkFramebuffer                    m_framebuffer;

    Graphics                         m_graphicsPipeline;

    PushBlockPrefilterEnv            m_pushBlock;
};