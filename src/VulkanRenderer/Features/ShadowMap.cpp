#include "VulkanRenderer/Features/ShadowMap.h"

#include <memory>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Descriptor/DescriptorSets.h"
#include "VulkanRenderer/Descriptor/DescriptorPool.h"
#include "VulkanRenderer/Descriptor/DescriptorTypes.h"
#include "VulkanRenderer/Framebuffer/FramebufferManager.h"
#include "VulkanRenderer/Math/MathUtils.h"
#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/Model/Attributes.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"

#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Renderer.h" 

template<typename T>
ShadowMap<T>::ShadowMap(
    const VkExtent2D& extent,
    const uint32_t imagesCount,
    const VkFormat& format,
    const uint32_t& uboCount,
    const std::vector<Mesh<T>>* meshes,
    const std::vector<size_t>& modelIndices
) :  m_width(extent.width), m_height(extent.height), m_modelIndices(modelIndices) {

 
    m_texture = std::make_shared<NormalTexture>("ShadowMap");
    m_texture->getFormat() = format;
    m_texture->getExtent() = VkExtent2D({m_width,m_height});


    BufferManager::bufferCreateDepthResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(),
        m_texture->getExtent(),
        format,
        VK_SAMPLE_COUNT_1_BIT,
        &m_texture->getImage(),
        &m_texture->getAllocation(),
        &m_texture->getImageView()
    );


    BufferManager::bufferCreateTextureSampler(
        getRendererPointer()->getDevice(),
        1,
        VK_FILTER_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        &m_texture->getSampler()
    );

    createUBO( uboCount);
    createRenderPass(format);
    createFramebuffer(imagesCount);
    createGraphicsPipeline(extent);
    createDescriptorPool();
    createDescriptorSets();
}

template<typename T>
void ShadowMap<T>::createGraphicsPipeline(const VkExtent2D& extent)
{
    m_graphicsPipeline = Graphics(
        getRendererPointer()->getDevice(),
        GraphicsPipelineType::SHADOWMAP,
        extent,
        m_renderPass,
        { {shaderType::VERTEX, "shadowMap"} },
        VK_SAMPLE_COUNT_1_BIT,
        Attributes::PBR::getBindingDescription(),
        Attributes::SHADOWMAP::getAttributeDescriptions(),
        m_modelIndices,
        GRAPHICS_PIPELINE::SHADOWMAP::UBOS_INFO,
        {},
        {}
    );
}

template<typename T>
ShadowMap<T>::~ShadowMap() {}

template<typename T>
void ShadowMap<T>::createCommandPool(const VkCommandPoolCreateFlags& flags, const uint32_t& graphicsFamilyIndex)
{
    m_commandPool = std::make_shared<CommandPool>(getRendererPointer()->getDevice(), flags, graphicsFamilyIndex);
}

template<typename T>
void ShadowMap<T>::updateUBO(
    const glm::mat4 modelM,
    const glm::fvec4 directionalLightStartPos,
    const glm::fvec4 directionalLightEndPos,
    const float aspect,
    const float zNear,
    const float zFar,
    const uint32_t& currentFrame,
    size_t  index
) {
    // TODO: Improve this.
    // The model and projection matrix don't need to be updated every frame.
    m_basicInfo.model = modelM;
    glm::mat4 proj = MathUtils::getUpdatedProjMatrix(glm::radians(Config::FOV), aspect, zNear, zFar);
    //glm::mat4 proj = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);

    proj[1][1] *= -1;

    glm::fvec3 lightDir = glm::normalize(glm::fvec3(directionalLightEndPos) - glm::fvec3(directionalLightStartPos));

    //glm::mat4 view = glm::lookAt(glm::fvec3(directionalLightStartPos),glm::fvec3(directionalLightEndPos),glm::fvec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::fvec3(directionalLightStartPos), glm::fvec3(directionalLightStartPos) + lightDir, glm::fvec3(0.0f, 0.0f, 1.0f));

    m_basicInfo.lightSpace = proj * view;


    void* data;
    vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocations[index], &data);
        memcpy(data, &m_basicInfo, sizeof(m_basicInfo));
    vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocations[index]);

}

template<typename T>
const VkCommandBuffer& ShadowMap<T>::getCommandBuffer(const uint32_t index) const
{
    return m_commandPool->getCommandBuffer(index);
}

template<typename T>
void ShadowMap<T>::allocCommandBuffers(const uint32_t& commandBuffersCount)
{
    m_commandPool->allocCommandBuffers(commandBuffersCount);
}

template<typename T>
void ShadowMap<T>::createDescriptorPool()
{
    m_descriptorPool = DescriptorPool(
        getRendererPointer()->getDevice(),
        { {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (uint32_t)m_modelIndices.size() * Config::MAX_FRAMES_IN_FLIGHT * GRAPHICS_PIPELINE::SHADOWMAP::UBOS_COUNT} },
        m_modelIndices.size() * Config::MAX_FRAMES_IN_FLIGHT * GRAPHICS_PIPELINE::SHADOWMAP::UBOS_COUNT
    );
}

template<typename T>
void ShadowMap<T>::createUBO(const uint32_t& uboCount)
{
    BufferManager::bufferCreateUniformBuffers(
        getRendererPointer()->getVmaAllocator(),
        m_modelIndices.size(),
        sizeof(DescriptorTypes::UniformBufferObject::ShadowMap),
        m_ubos,
        m_uboAllocations
    );
}

template<typename T>
void ShadowMap<T>::createDescriptorSets()
{
    m_descriptorSets.resize(m_modelIndices.size());

    for (uint32_t i = 0; i < m_modelIndices.size(); i++ )
    {
        m_descriptorSets[i] = DescriptorSets(
            getRendererPointer()->getDevice(),
            GRAPHICS_PIPELINE::SHADOWMAP::UBOS_INFO,
            {},
            {},
            m_graphicsPipeline.getDescriptorSetLayout(),
            m_descriptorPool,
            nullptr,
            { m_ubos[i] }
        );
    }
}

template<typename T>
const std::shared_ptr<TextureBase> ShadowMap<T>::get() const
{
    return m_texture;
}

template<typename T>
const VkImageView& ShadowMap<T>::getShadowMapView() const
{
    return m_texture->getImageView();
}

template<typename T>
const VkSampler& ShadowMap<T>::getSampler() const
{
    return m_texture->getSampler();
}

template<typename T>
void ShadowMap<T>::bindData(const std::vector<Mesh<T>>* meshes, const size_t index, const VkCommandBuffer& commandBuffer, const uint32_t currentFrame)
{
    for (auto mesh = meshes->begin(); mesh != meshes->end(); mesh++)
    {
        CommandManager::STATE::bindVertexBuffers(
            { mesh->vertexBuffer },
            // Offsets.
            { 0 },
            // Index of first binding.
            0,
            // Bindings count.
            1,
            commandBuffer
        );
        CommandManager::STATE::bindIndexBuffer(
            mesh->indexBuffer,
            // Offset.
            0,
            VK_INDEX_TYPE_UINT32,
            commandBuffer
        );

        CommandManager::STATE::bindDescriptorSets(
            m_graphicsPipeline.getPipelineLayout(),
            PipelineType::GRAPHICS,
            // Index of first descriptor set.
            0,
            { getDescriptorSet(index,currentFrame) },
            // Dynamic offsets.
            {},
            commandBuffer
        );

        CommandManager::ACTION::drawIndexed(
            // Index Count
            mesh->indices.size(),
            // Instance Count
            1,
            // First index.
            0,
            // Vertex Offset.
            0,
            // First Intance.
            0,
            commandBuffer
        );
    }
}


template<typename T>
const VkDescriptorSet& ShadowMap<T>::getDescriptorSet(const size_t index, const uint32_t currentFrame) const
{
    VkDescriptorSet ret = m_descriptorSets[index].get(currentFrame);
    return ret;
}

template<typename T>
const VkFramebuffer& ShadowMap<T>::getFramebuffer(const uint32_t imageIndex) const
{
    return m_framebuffers[imageIndex];
}

template<typename T>
const std::shared_ptr<CommandPool>& ShadowMap<T>::getCommandPool() const
{
    return m_commandPool;
}

template<typename T>
const glm::mat4& ShadowMap<T>::getLightSpace() const
{
    return m_basicInfo.lightSpace;
}

template<typename T>
const RenderPass& ShadowMap<T>::getRenderPass() const
{
    return m_renderPass;
}

template<typename T>
const Graphics& ShadowMap<T>::getGraphicsPipeline() const
{
    return m_graphicsPipeline;
}

template<typename T>
void ShadowMap<T>::createFramebuffer(const uint32_t& imagesCount)
{

    m_framebuffers.resize(imagesCount);

    // We'll write in the sampler to later use it in the scene fragment shader.
    std::vector<VkImageView> attachments = { m_texture->getImageView() };

    for (uint32_t i = 0; i < imagesCount; i++)
    {
        FramebufferManager::createFramebuffer(getRendererPointer()->getDevice(), m_renderPass.get(), attachments, m_width, m_height, 1, m_framebuffers[i]);
    }
}

template<typename T>
void ShadowMap<T>::destroy()
{
    m_graphicsPipeline.destroy();
    m_descriptorPool.destroy();
    m_texture->destroy();

    for (size_t i = 0; i < m_ubos.size(); i++)
    {
        vmaDestroyBuffer(getRendererPointer() ->getVmaAllocator(), m_ubos[i], m_uboAllocations[i]);
    }

    m_commandPool->destroy();

    m_renderPass.destroy();

    for (auto& framebuffer : m_framebuffers)
        vkDestroyFramebuffer(getRendererPointer()->getDevice(), framebuffer, nullptr);
}

template<typename T>
void ShadowMap<T>::createRenderPass(const VkFormat& depthBufferFormat)
{
    // - Attachments
    VkAttachmentDescription shadowMapAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        depthBufferFormat,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        shadowMapAttachment
    );


    // Attachment references
    VkAttachmentReference shadowMapAttachmentRef{};
    AttachmentUtils::createAttachmentReference(
        0,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        shadowMapAttachmentRef
    );

    // Subpasses
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.pDepthStencilAttachment = &shadowMapAttachmentRef;

    m_renderPass = RenderPass(
        getRendererPointer()->getDevice(),
        { shadowMapAttachment },
        { subpass },
        {}
        //dependencies
    );

}


////////////////////////////////////INSTANCES//////////////////////////////////
template class ShadowMap<Attributes::PBR::Vertex>;