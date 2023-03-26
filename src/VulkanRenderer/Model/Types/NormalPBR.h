#pragma once

#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/Model/ModelInfo.h"
#include "VulkanRenderer/Descriptor/DescriptorTypes.h"
#include "VulkanRenderer/Features/ShadowMap.h"


class NormalPBR : public Model
{
public:
    NormalPBR(const ModelInfo& modelInfo);

    ~NormalPBR() override;

    void destroy() override;

    void createDescriptorSets(
        const VkDescriptorSetLayout& descriptorSetLayout,
        std::vector<VkDescriptorImageInfo*> info,
        VkDescriptorPool& descriptorPool
    )override;

    void bindData(
        const Graphics* graphicsPipeline,
        const VkCommandBuffer& commandBuffer
    )override;

    void updateUBO(
        const uint32_t& currentFrame,
        const UBOinfo& uboInfo
    ) override;

    void updateUBOlights(
        const uint32_t& currentFrame
    );

    const glm::mat4& getModelM() const;
    const std::vector<Mesh<Attributes::PBR::Vertex>>& getMeshes() const;

private:

    void processMesh(aiMesh* mesh, const aiScene* scene) override;
    void getMaterialTextureInfo(
        aiMaterial* material,
        const aiTextureType& type,
        const std::string& typeName,
        const std::string& defaultTextureFile,
        TextureToLoadInfo& info
    );

    void uploadVertexData(
        const VkQueue& graphicsQueue,
        const VkCommandPool& commandPool
    ) override;

    void uploadTextures(
        const VkSampleCountFlagBits& samplesCount,
        const VkCommandPool& commandPool,
        const VkQueue& graphicsQueue
    ) override;

    void createUniformBuffers(
        const uint32_t& uboCount
    ) override;

    VkBuffer                m_uboLight;
    VmaAllocation           m_uboLightAllocation;

    DescriptorTypes::UniformBufferObject::NormalPBR m_dataInShader;
    DescriptorTypes::UniformBufferObject::LightInfo m_lightsInfo[Config::LIGHTS_COUNT];
    std::vector<Mesh<Attributes::PBR::Vertex>> m_meshes;
};