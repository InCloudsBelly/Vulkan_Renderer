#pragma once

#include "VulkanRenderer/Model/Model.h"
#include "VulkanRenderer/Descriptors/Types/DescriptorTypes.h"

class NormalPBR : public Model
{
public:

	NormalPBR(const std::string& name, const std::string& modelFileName,
        const glm::fvec4& pos = glm::fvec4(0.0f),
        const glm::fvec3& rot = glm::fvec3(0.0f),
        const glm::fvec3& size = glm::fvec3(1.0f)
    );

	~NormalPBR() override;

    void destroy(const VkDevice& logicalDevice) override;

    void createDescriptorSets(
        const VkDevice& logicalDevice,
        const VkDescriptorSetLayout& descriptorSetLayout,
        DescriptorPool& descriptorPool
    ) override;

    void createUniformBuffers(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const uint32_t& uboCount
    ) override;

    void uploadVertexData(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        VkQueue& graphicsQueue,
        CommandPool& commandPool
    ) override;

    void createTextures(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        CommandPool& commandPool,
        VkQueue& graphicsQueue
    ) override;

    void updateUBO(
        const VkDevice& logicalDevice,
        const glm::vec4& cameraPos,
        const glm::mat4& view,
        const glm::mat4& proj,
        const std::vector<std::shared_ptr<Model>>& models,
        const std::vector<size_t> directionalLightIndices,
        const uint32_t& currentFrame
    );

    // Info to update UBO.
   float extremeX[2];
   float extremeY[2];
   float extremeZ[2];

   std::vector<Mesh<Attributes::PBR::Vertex>> m_meshes;

private:

   void processMesh(aiMesh* mesh, const aiScene* scene) override;
   std::string getMaterialTextureName(
      aiMaterial* material,
      const aiTextureType& type,
      const std::string& typeName
   );

   void updateLightData(
       DescriptorTypes::UniformBufferObject::NormalPBR& ubo,
       const std::vector<std::shared_ptr<Model>>& models,
       const std::vector<size_t> directionalLightIndices
   );
};