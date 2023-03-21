#include "VulkanRenderer/Model/Types/Light.h"

#include "VulkanRenderer/Settings/graphicsPipelineConfig.h"
#include "VulkanRenderer/Descriptor/DescriptorTypes.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Math/MathUtils.h"
#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Command/CommandManager.h"

#include "VulkanRenderer/Renderer.h"

Light::Light(const ModelInfo& modelInfo)
    : Model(modelInfo.name, modelInfo.folderName, ModelType::LIGHT, glm::fvec4(modelInfo.pos, 1.0f), modelInfo.rot, modelInfo.size),
    m_targetPos(glm::fvec4(modelInfo.endPos, 1.0f)),
    m_color(glm::fvec4(modelInfo.color, 1.0f)),
    m_lightType(modelInfo.lType)
{
    if (modelInfo.lType == LightType::DIRECTIONAL_LIGHT)
        m_intensity = 15.0f;
    else
        m_intensity = 70.0f;

    loadModel((std::string(MODEL_DIR) + modelInfo.folderName + "/" + modelInfo.fileName).c_str());
    m_rot = glm::fvec3(0.0f);
}

Light::~Light() {}

void Light::destroy(const VkDevice& logicalDevice)
{
    vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_ubo, m_uboAllocation);

    for (auto& texture : m_texturesLoaded)
        texture->destroy();

    for (auto& mesh : m_meshes)
    {
  vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), mesh.vertexBuffer, mesh.vertexAllocation);
        vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), mesh.indexBuffer, mesh.indexAllocation);
    }
}

void Light::processMesh(aiMesh* mesh, const aiScene* scene)
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

        vertex.texCoord = {
           mesh->mTextureCoords[0][i].x,
           mesh->mTextureCoords[0][i].y,
        };

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

void Light::createUniformBuffers(const VkPhysicalDevice& physicalDevice, const VkDevice& logicalDevice, const uint32_t& uboCount)
{
    BufferManager::bufferCreateBuffer(
        getRendererPointer()->getVmaAllocator(),
        sizeof(DescriptorTypes::UniformBufferObject::Light),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        &m_ubo,
        &m_uboAllocation
    );
}

void Light::createDescriptorSets(const VkDevice& logicalDevice, const VkDescriptorSetLayout& descriptorSetLayout, DescriptorSetInfo* info, DescriptorPool& descriptorPool)
{
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
            { m_ubo }
        );
    }
}

void Light::bindData(
    const Graphics* graphicsPipeline,
    const VkCommandBuffer& commandBuffer,
    const uint32_t currentFrame
) {
    for (auto& mesh : m_meshes)
    {
        std::vector<VkBuffer> vertexBuffers = { mesh.vertexBuffer };
        std::vector<VkDeviceSize> offsets = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        const std::vector<VkDescriptorSet> sets = { mesh.descriptorSets.get(currentFrame) };
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline->getPipelineLayout(),
            // Index of the first descriptor set.
            0,
            sets.size(), sets.data(),
            0, {}
        );

        vkCmdDrawIndexed(commandBuffer, mesh.indices.size(), 1, 0, 0, 0);
    }
}

void Light::uploadVertexData(const VkPhysicalDevice& physicalDevice, const VkDevice& logicalDevice, const VkQueue& graphicsQueue, const std::shared_ptr<CommandPool>& commandPool)
{

    for (auto& mesh : m_meshes)
    {
        BufferManager::createBufferAndTransferToDevice(
            getRendererPointer()->getDevice(),
            getRendererPointer()->getVmaAllocator(),
            graphicsQueue,
            commandPool->get(),
            mesh.vertices.data(),
            sizeof(mesh.vertices[0]) * mesh.vertices.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            &mesh.vertexBuffer,
            &mesh.vertexAllocation
        );

        BufferManager::createBufferAndTransferToDevice(
            getRendererPointer()->getDevice(),
            getRendererPointer()->getVmaAllocator(),
            graphicsQueue,
            commandPool->get(),
            mesh.indices.data(),
            sizeof(mesh.indices[0]) * mesh.indices.size(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            &mesh.indexBuffer,
            &mesh.indexAllocation
        );
    }
}

void Light::uploadTextures(const VkPhysicalDevice& physicalDevice, const VkDevice& logicalDevice, const VkSampleCountFlagBits& samplesCount, const std::shared_ptr<CommandPool>& commandPool, const VkQueue& graphicsQueue)
{
    const size_t nTextures = GRAPHICS_PIPELINE::LIGHT::TEXTURES_PER_MESH_COUNT;
    const TextureToLoadInfo info = { "DefaultTexture.png", "defaultTextures",VK_FORMAT_R8G8B8A8_SRGB , 4 };

    for (auto& mesh : m_meshes)
    {
        for (size_t i = 0; i < nTextures; i++)
        {
            auto it = m_texturesID.find(info.name);

            if (it == m_texturesID.end())
            {
                mesh.textures.push_back(std::make_shared<NormalTexture>(info.name, std::string(MODEL_DIR) + info.folderName, info.format));

                m_texturesLoaded.push_back(mesh.textures[i]);
                m_texturesID[info.name] = m_texturesLoaded.size() - 1;
            }
            else
                mesh.textures.push_back(m_texturesLoaded[it->second]);

        }
    }
}

void Light::updateUBO(const VkDevice& logicalDevice, const uint32_t& currentFrame, const UBOinfo& uboInfo)
{

    m_dataInShader.model = MathUtils::getUpdatedModelMatrix(m_pos, m_rot, m_size);

    m_dataInShader.view = uboInfo.view;
    m_dataInShader.proj = uboInfo.proj;
    m_dataInShader.lightColor = m_color;

    void* data;
    vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocation, &data);
        memcpy(data, &m_dataInShader, sizeof(m_dataInShader));
    vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocation);

}

const glm::fvec4& Light::getColor() const
{
    return m_color;
}

const float& Light::getIntensity() const
{
    return m_intensity;
}

const glm::fvec4& Light::getTargetPos() const
{
    return m_targetPos;
}

const LightType& Light::getLightType() const
{
    return m_lightType;
}

void Light::setColor(const glm::fvec4& newColor)
{
    m_color = newColor;
}

void Light::setTargetPos(const glm::fvec4& pos)
{
    m_targetPos = pos;
}

void Light::setIntensity(const float& intensity)
{
    m_intensity = intensity;
}
