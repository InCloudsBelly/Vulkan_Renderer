#pragma once

#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/Model/ModelInfo.h"
#include "VulkanRenderer/Features/ShadowMap.h"

#include <GLFW/glfw3.h>

class Light : public Model
{
public:

    Light(const ModelInfo& modelInfo);

    ~Light() override;

    void destroy() override;

    void createDescriptorSets(const VkDescriptorSetLayout& descriptorSetLayout, std::vector<VkDescriptorImageInfo*> info, VkDescriptorPool& descriptorPool)override;

    void bindData(
        const Graphics* graphicsPipeline,
        const VkCommandBuffer& commandBuffer
    ) override;

    void updateUBO(
        const uint32_t& currentFrame,
        const UBOinfo& uboInfo
    )override;

    const glm::fvec4& getColor() const;
    const glm::fvec4& getTargetPos() const;
    const float& getIntensity() const;
    const LightType& getLightType() const;

    void setColor(const glm::fvec4& newColor);
    void setIntensity(const float& intensity);
    void setTargetPos(const glm::fvec4& pos);

private:
    
    void createUniformBuffers(
        const uint32_t& uboCount
    ) override;
    
    void uploadVertexData(
        const VkQueue& graphicsQueue,
        const VkCommandPool& commandPool
    ) override;
    
    void uploadTextures(
        const VkSampleCountFlagBits& samplesCount,
        const VkCommandPool& commandPool,
        const VkQueue& graphicsQueue
    ) override;

    void processMesh(aiMesh* mesh, const aiScene* scene) override;

    // To get the direction of directional and spot lights(m_endPos - m_Pos);
    glm::fvec4 m_targetPos;
    glm::fvec4 m_color;
    float      m_intensity;
    LightType  m_lightType;

    std::vector<Mesh<Attributes::LIGHT::Vertex>> m_meshes;
    DescriptorTypes::UniformBufferObject::Light m_dataInShader;
};