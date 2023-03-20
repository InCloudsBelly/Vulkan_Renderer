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

    void destroy(const VkDevice& logicalDevice) override;

    void createDescriptorSets(
        const VkDevice& logicalDevice,
        const VkDescriptorSetLayout& descriptorSetLayout,
        DescriptorSetInfo* info,
        DescriptorPool& descriptorPool
    )override;

    void bindData(
        const Graphics* graphicsPipeline,
        const VkCommandBuffer& commandBuffer,
        const uint32_t currentFrame
    )override;

    void updateUBO(
        const VkDevice& logicalDevice,
        const uint32_t& currentFrame,
        const UBOinfo& uboInfo
    ) override;

    void updateUBOlights(
        const VkDevice& logicalDevice,
        const std::vector<size_t> lightModelIndices,
        const std::vector<std::shared_ptr<Model>>& models,
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
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const VkQueue& graphicsQueue,
        const std::shared_ptr<CommandPool>& commandPool
    ) override;

    void uploadTextures(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const VkSampleCountFlagBits& samplesCount,
        const std::shared_ptr<CommandPool>& commandPool,
        const VkQueue& graphicsQueue
    ) override;

    void createUniformBuffers(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const uint32_t& uboCount
    ) override;

    VkBuffer                m_uboLight;
    VmaAllocation           m_uboLightAllocation;

    DescriptorTypes::UniformBufferObject::NormalPBR m_dataInShader;
    DescriptorTypes::UniformBufferObject::LightInfo m_lightsInfo[Config::LIGHTS_COUNT];
    std::vector<Mesh<Attributes::PBR::Vertex>> m_meshes;
};