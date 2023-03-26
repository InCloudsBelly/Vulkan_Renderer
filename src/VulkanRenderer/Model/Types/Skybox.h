#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/Model/Attributes.h"
#include "VulkanRenderer/Model/ModelInfo.h"

#include "VulkanRenderer/Texture/Texture.h"

#include "VulkanRenderer/Features/ShadowMap.h"

class Skybox : public Model
{
public:

    Skybox(const ModelInfo& modelInfo);

	~Skybox() override;

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

    const std::string& getTextureFolderName() const;

    const std::shared_ptr<TextureBase>& getEnvMap() const;

    const std::vector<Mesh<Attributes::SKYBOX::Vertex>>& getMeshes() const;

private:

    void processMesh(aiMesh* mesh, const aiScene* scene) override;

    void uploadVertexData(
        const VkQueue& graphicsQueue,
        const VkCommandPool& commandPool
    )override;
    void uploadTextures(
        const VkSampleCountFlagBits& samplesCount,
        const VkCommandPool& commandPool,
        const VkQueue& graphicsQueue
    ) override;
    void createUniformBuffers(
        const uint32_t& uboCount
    ) override;

    std::string                     m_textureFolderName;
    std::shared_ptr<TextureBase>    m_envMap;
    std::vector<Mesh<Attributes::SKYBOX::Vertex>> m_meshes;
};