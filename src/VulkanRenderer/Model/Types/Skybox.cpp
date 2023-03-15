#include "VulkanRenderer/Model/Types/Skybox.h"

#include <string>
#include <vector>
#include <cstring>

#include <vulkan/vulkan.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Descriptor/DescriptorSetLayoutManager.h"
#include "VulkanRenderer/Pipeline/Graphics.h"
#include "VulkanRenderer/Model/Attributes.h"

#include "VulkanRenderer/Descriptor/Types/DescriptorTypes.h"
#include "VulkanRenderer/Descriptor/Types/UBO/UBO.h"
#include "VulkanRenderer/Descriptor/Types/UBO/UBOutils.h"
#include "VulkanRenderer/Descriptor/DescriptorSets.h"
#include "VulkanRenderer/Descriptor/DescriptorPool.h"

#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Math/MathUtils.h"

#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Command/CommandManager.h"

Skybox::Skybox(const ModelInfo& modelInfo)
    : Model(modelInfo.name, modelInfo.folderName, ModelType::SKYBOX)
{
    loadModel((std::string(MODEL_DIR) + "cubeDefault/Cube.gltf").c_str());

}

void Skybox::destroy(const VkDevice& logicalDevice)
{
	m_ubo->destroy();

    for (auto& texture : m_texturesLoaded)
        texture->destroy();

    for (auto& mesh : m_meshes)
    {
        BufferManager::destroyBuffer(logicalDevice,mesh.vertexBuffer);
        BufferManager::destroyBuffer(logicalDevice,mesh.indexBuffer);

        BufferManager::freeMemory(logicalDevice, mesh.vertexMemory);
        BufferManager::freeMemory(logicalDevice, mesh.indexMemory);
    }
}


void Skybox::processMesh(aiMesh* mesh, const aiScene* scene)
{
    Mesh<Attributes::SKYBOX::Vertex> newMesh;

    for (size_t i = 0; i < mesh->mNumVertices; i++)
    {
        Attributes::SKYBOX::Vertex vertex{};
        vertex.pos = {mesh->mVertices[i].x,mesh->mVertices[i].y,mesh->mVertices[i].z};
        newMesh.vertices.emplace_back(vertex);
    }

    for (size_t i = 0; i < mesh->mNumFaces; i++)
    {
        auto face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j++)
            newMesh.indices.emplace_back(face.mIndices[j]);
    }

    m_meshes.emplace_back(newMesh);
}

Skybox::~Skybox() {}

void Skybox::createDescriptorSets(const VkDevice& logicalDevice, const VkDescriptorSetLayout& descriptorSetLayout, DescriptorSetInfo* info, DescriptorPool& descriptorPool)
{
    std::vector<UBO*> opUBOs = { m_ubo.get()};

    for (auto& mesh : m_meshes)
    {
        mesh.descriptorSets = DescriptorSets(
            logicalDevice,
            GRAPHICS_PIPELINE::SKYBOX::UBOS_INFO,
            GRAPHICS_PIPELINE::SKYBOX::SAMPLERS_INFO,
            mesh.textures,
            descriptorSetLayout,
            descriptorPool,
            nullptr,
            opUBOs
        );
    }
}

void Skybox::createUniformBuffers(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice,const uint32_t& uboCount) 
{
    m_ubo = std::make_shared<UBO>(
        physicalDevice,
        logicalDevice,
        uboCount,
        sizeof(DescriptorTypes::UniformBufferObject::Skybox)
    );
}

void Skybox::uploadVertexData(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice, const VkQueue& graphicsQueue, const std::shared_ptr<CommandPool>& commandPool)
{
    for (auto& mesh : m_meshes)
    {
        // Vertex Buffer(with staging buffer)
        BufferManager::createBufferAndTransferToDevice(
            commandPool,
            physicalDevice,
            logicalDevice,
            mesh.vertices.data(),
            sizeof(mesh.vertices[0]) * mesh.vertices.size(),
            graphicsQueue,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            mesh.vertexMemory,
            mesh.vertexBuffer
        );

        // Index Buffer(with staging buffer)
        BufferManager::createBufferAndTransferToDevice(
            commandPool,
            physicalDevice,
            logicalDevice,
            mesh.indices.data(),
            sizeof(mesh.indices[0]) * mesh.indices.size(),
            graphicsQueue,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            mesh.indexMemory,
            mesh.indexBuffer
        );
    }
}

void Skybox::uploadTextures(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice, const VkSampleCountFlagBits& samplesCount, const std::shared_ptr<CommandPool>& commandPool, const VkQueue& graphicsQueue)
{
    const size_t nTextures = GRAPHICS_PIPELINE::SKYBOX::TEXTURES_PER_MESH_COUNT;
    TextureToLoadInfo info = {m_name, m_folderName, VK_FORMAT_R32G32B32A32_SFLOAT, 4 };

    for (auto& mesh : m_meshes)
    {
        for (size_t i = 0; i < nTextures; i++)
        {
            auto it = (m_texturesID.find(info.name));

            if (it == m_texturesID.end())
            {
                //mesh.texturesCube.push_back(std::make_shared<Cubemap>(physicalDevice,logicalDevice, info, samplesCount,commandPool,graphicsQueue, UsageType::ENVIRONMENTAL_MAP));
                mesh.textures.push_back(std::make_shared<CubeMapTexture>(info.name, info.folderName, info.format));
                
                m_texturesLoaded.push_back(mesh.textures[i]);
                m_texturesID[info.name] = (m_texturesLoaded.size() - 1);

                m_envMap = mesh.textures[i];
            }
            else
                mesh.textures.push_back(m_texturesLoaded[it->second]);
        }
    }


}

void Skybox::updateUBO(
    const VkDevice& logicalDevice,
    const uint32_t& currentFrame,
    const UBOinfo& uboInfo
) {
    DescriptorTypes::UniformBufferObject::Skybox newUBO;

    newUBO.model = glm::translate(glm::mat4(1.0f),glm::vec3(uboInfo.cameraPos));
    newUBO.view = uboInfo.view;
    newUBO.proj = MathUtils::getUpdatedProjMatrix(glm::radians(75.0f), uboInfo.extent.width / (float)uboInfo.extent.height,0.01f,40.0f);

    const size_t size = sizeof(newUBO);
    UBOutils::updateUBO(logicalDevice, m_ubo, size, &newUBO, currentFrame);
}

void Skybox::bindData(const Graphics* graphicsPipeline, const VkCommandBuffer& commandBuffer, const uint32_t currentFrame)
{
    for (auto& mesh : m_meshes)
    {
        CommandManager::STATE::bindVertexBuffers({ mesh.vertexBuffer }, { 0 }, 0, 1, commandBuffer);
        CommandManager::STATE::bindIndexBuffer({ mesh.indexBuffer }, 0, VK_INDEX_TYPE_UINT32, commandBuffer);
        CommandManager::STATE::bindDescriptorSets(graphicsPipeline->getPipelineLayout(), PipelineType::GRAPHICS, 0, { mesh.descriptorSets.get(currentFrame) }, {}, commandBuffer);

        CommandManager::ACTION::drawIndexed(mesh.indices.size(), 1, 0, 0, 0, commandBuffer);
    }
}

const std::string& Skybox::getTextureFolderName() const
{
    return m_folderName;
}


const std::shared_ptr<TextureBase>& Skybox::getEnvMap() const 
{
    return m_envMap;
}

const std::vector<Mesh<Attributes::SKYBOX::Vertex>>& Skybox::getMeshes() const
{
    return m_meshes;
}
