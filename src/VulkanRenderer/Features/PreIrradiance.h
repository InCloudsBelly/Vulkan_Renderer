#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/Image/Image.h"
#include "VulkanRenderer/RenderPass/RenderPass.h"
#include "VulkanRenderer/Descriptor/DescriptorSets.h"
#include "VulkanRenderer/Pipeline/Graphics.h"
#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/Texture/Texture.h"

#define M_PI       3.14159265358979323846

struct PushBlockIrradiance {
    glm::mat4 mvp;
    float deltaPhi = (2.0f * M_PI) / 180.0f;
    float deltaTheta = (0.5f * M_PI) / 64.0f;
};


template<typename T>
class PrefilteredIrradiance
{
public:

    PrefilteredIrradiance(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const VkQueue& graphicsQueue,
        const std::shared_ptr<CommandPool>& commandPool,
        const uint32_t dim,
        const std::vector<Mesh<T>>& meshes,
        const std::shared_ptr<TextureBase>& envMap
    );
    ~PrefilteredIrradiance();
    void destroy();
    const Image& get() const;

private:

    void createPipeline();
    void createOffscreenFramebuffer(
        const VkPhysicalDevice& physicalDevice,
        const VkQueue& graphicsQueue,
        const std::shared_ptr<CommandPool>& commandPool
    );
    void createRenderPass();
    void createTargetImage(const VkPhysicalDevice& physicalDevice);
    void createDescriptorPool();
    void createDescriptorSet(const std::shared_ptr<TextureBase>& envMap);
    void copyRegionOfImage(
        float face,
        float mipLevel,
        float viewportDim,
        const VkCommandBuffer& commandBuffer
    );
    void recordCommandBuffer(
        const std::shared_ptr<CommandPool>& commandPool,
        const VkQueue& graphicsQueue,
        const std::vector<Mesh<T>>& meshes
    );

    VkDevice                         m_logicalDevice;

    uint32_t                         m_dim;
    VkFormat                         m_format;
    uint32_t                         m_mipLevels;

    Image                            m_targetImage;
    Image                            m_offscreenImage;

    RenderPass                       m_renderPass;

    DescriptorSets                   m_descriptorSets;
    DescriptorPool                   m_descriptorPool;

    VkFramebuffer                    m_framebuffer;

    Graphics                         m_graphicsPipeline;

    PushBlockIrradiance             m_pushBlock;
};