#include "VulkanRenderer/Model/Types/DirectionalLight.h"

#include "VulkanRenderer/Settings/config.h"
#include "VulkanRenderer/Descriptors/Types/DescriptorTypes.h"
#include "VulkanRenderer/Descriptors/Types/UBO/UBOutils.h"
#include "VulkanRenderer/Settings/graphicsPipelineConfig.h"
#include "VulkanRenderer/Buffers/BufferManager.h"

DirectionalLight::DirectionalLight(
    const std::string& name,
    const std::string& modelFileName
) : Model(name, ModelType::DIRECTIONAL_LIGHT) {

    loadModel((std::string(MODEL_DIR) + modelFileName).c_str());

    for (auto& mesh : m_meshes)
    {
        mesh.m_textures.resize(GRAPHICS_PIPELINE::LIGHT::TEXTURES_PER_MESH_COUNT);
        mesh.m_texturesToLoadInfo.push_back({"textures/default.jpg",VK_FORMAT_R8G8B8A8_SRGB});
    }

    // TODO: Load scene settings.
    actualPos = glm::fvec4(0.0f);
    actualSize = glm::fvec3(1.0f);
    actualRot = glm::fvec3(0.0f);
}

DirectionalLight::~DirectionalLight() {}

void DirectionalLight::destroy(const VkDevice& logicalDevice)
{
    m_ubo.destroyUniformBuffersAndMemories(logicalDevice);

    for (auto& mesh : m_meshes)
    {
        for (auto& texture : mesh.m_textures)
            texture.destroyTexture(logicalDevice);

        BufferManager::destroyBuffer(logicalDevice,mesh.m_vertexBuffer);
        BufferManager::destroyBuffer(logicalDevice,mesh.m_indexBuffer);

        BufferManager::freeMemory(logicalDevice,mesh.m_vertexMemory);
        BufferManager::freeMemory(logicalDevice,mesh.m_indexMemory);
    }
}

void DirectionalLight::processMesh(aiMesh* mesh, const aiScene* scene)
{
    Mesh<Attributes::LIGHT::Vertex> newMesh;

    for (size_t i = 0; i < mesh->mNumVertices; i++)
    {
        Attributes::LIGHT::Vertex vertex{};

        vertex.pos = {
           mesh->mVertices[i].x,
           mesh->mVertices[i].y,
           mesh->mVertices[i].z
        };

        vertex.normal = {
           mesh->mNormals[i].x,
           mesh->mNormals[i].y,
           mesh->mNormals[i].z
        };
        vertex.normal = glm::normalize(vertex.normal);

        vertex.texCoord = {
           mesh->mTextureCoords[0][i].x,
           mesh->mTextureCoords[0][i].y,
        };

        newMesh.m_vertices.push_back(vertex);

    }

    for (size_t i = 0; i < mesh->mNumFaces; i++)
    {
        auto face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j++)
            newMesh.m_indices.push_back(face.mIndices[j]);
    }

    m_meshes.push_back(newMesh);
}

void DirectionalLight::createUniformBuffers(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice,const uint32_t& uboCount) 
{
    m_ubo.createUniformBuffers(physicalDevice, logicalDevice, uboCount, sizeof(DescriptorTypes::UniformBufferObject::Light));
}

void DirectionalLight::createDescriptorSets(const VkDevice& logicalDevice,const VkDescriptorSetLayout& descriptorSetLayout,DescriptorPool& descriptorPool) 
{
    for (auto& mesh : m_meshes)
    {
        mesh.m_descriptorSets.createDescriptorSets(
            logicalDevice,
            GRAPHICS_PIPELINE::LIGHT::UBOS_INFO,
            GRAPHICS_PIPELINE::LIGHT::SAMPLERS_INFO,
            mesh.m_textures,
            m_ubo.getUniformBuffers(),
            descriptorSetLayout,
            descriptorPool
        );
    }
}

void DirectionalLight::uploadVertexData(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice,VkQueue& graphicsQueue,CommandPool& commandPool) 
{

    for (auto& mesh : m_meshes)
    {
        // Vertex Buffer(with staging buffer)
        BufferManager::createBufferAndTransferToDevice(
            commandPool,
            physicalDevice,
            logicalDevice,
            mesh.m_vertices.data(),
            sizeof(mesh.m_vertices[0]) * mesh.m_vertices.size(),
            graphicsQueue,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            mesh.m_vertexMemory,
            mesh.m_vertexBuffer
        );

        // Index Buffer(with staging buffer)
        BufferManager::createBufferAndTransferToDevice(
            commandPool,
            physicalDevice,
            logicalDevice,
            mesh.m_indices.data(),
            sizeof(mesh.m_indices[0]) * mesh.m_indices.size(),
            graphicsQueue,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            mesh.m_indexMemory,
            mesh.m_indexBuffer
        );
    }
}

void DirectionalLight::createTextures(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice,CommandPool& commandPool,VkQueue& graphicsQueue) 
{
    for (auto& mesh : m_meshes)
    {
        for (size_t i = 0; i < mesh.m_textures.size(); i++)
        {
            mesh.m_textures[i] = Texture(
                physicalDevice,
                logicalDevice,
                mesh.m_texturesToLoadInfo[i],
                // isSkybox
                false,
                commandPool,
                graphicsQueue
            );
        }
    }
}

void DirectionalLight::updateUBO(const VkDevice& logicalDevice,const glm::vec4& cameraPos,const glm::mat4& proj,const uint32_t& currentFrame) 
{

    DescriptorTypes::UniformBufferObject::Light newUBO;

    newUBO.model = UBOutils::getUpdatedModelMatrix(actualPos, actualRot, actualSize);

    newUBO.view = UBOutils::getUpdatedViewMatrix(glm::vec3(cameraPos), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    newUBO.proj = proj;

    newUBO.lightColor = color;

    UBOutils::updateUBO(m_ubo, logicalDevice, newUBO, currentFrame);
}