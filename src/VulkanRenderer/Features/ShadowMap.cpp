#include "VulkanRenderer/Features/ShadowMap.h"

#include <memory>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Framebuffer/FramebufferManager.h"
#include "VulkanRenderer/Math/MathUtils.h"
#include "VulkanRenderer/Model/Attributes.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"

#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Descriptor/DescriptorTypes.h"
#include "VulkanRenderer/Descriptor/DescriptorManager.h"

#include "VulkanRenderer/Renderer.h" 

template<typename T>
ShadowMap<T>::ShadowMap(
    const VkExtent2D& extent,
    const uint32_t imagesCount,
    const VkFormat& format,
    const uint32_t& uboCount
) : m_width(extent.width), m_height(extent.height){

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

    //Create UBOs
    BufferManager::bufferCreateUniformBuffers(
        getRendererPointer()->getVmaAllocator(),
        getRenderResource()->m_objectModelIndices.size(),
        sizeof(DescriptorTypes::UniformBufferObject::ShadowMap),
        m_ubos,
        m_uboAllocations
    );


    createRenderPass(format);
    createFramebuffer(imagesCount);
    createGraphicsPipeline(extent);
    
    //create DescriptorPool
    std::vector<VkDescriptorPoolSize> poolSizes = {
       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(uint32_t)getRenderResource()->m_objectModelIndices.size() * GRAPHICS_PIPELINE::SHADOWMAP::UBOS_COUNT}
    };
    DescriptorManager::createDescriptorPool(poolSizes, &m_descriptorPool);


    //create DescriptorSets
    m_modelDescriptorSets.resize(getRenderResource()->m_objectModelIndices.size());
    
    for (uint32_t i = 0; i < getRenderResource()->m_objectModelIndices.size(); i++)
    {
        DescriptorManager::allocDescriptorSet(m_descriptorPool, m_graphicsPipeline.getDescriptorSetLayout(), &m_modelDescriptorSets[i]);
        
        DescriptorManager::createDescriptorSet(
            GRAPHICS_PIPELINE::SHADOWMAP::DESCRIPTORS_INFO,
            {},{},
            { m_ubos[i] },
            &m_modelDescriptorSets[i]
        );
    }

    //Create CommandPool
    CommandManager::cmdCreateCommandPool(
        getRendererPointer()->getDevice(),
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        getRendererPointer()->getQueueFamilyIndices().graphicsFamily.value(),
        &m_commandPool
    );

    //Create Commands
    m_commandBuffers.resize(Config::MAX_FRAMES_IN_FLIGHT);
    CommandManager::cmdCreateCommandBuffers(
        getRendererPointer()->getDevice(),
        m_commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        Config::MAX_FRAMES_IN_FLIGHT,
        &m_commandBuffers[0]
    );

}

template<typename T>
void ShadowMap<T>::createGraphicsPipeline(const VkExtent2D& extent)
{
    m_graphicsPipeline = Graphics(
        GraphicsPipelineType::SHADOWMAP,
        extent,
        m_renderPass,
        { {shaderType::VERTEX, "shadowMap"} },
        VK_SAMPLE_COUNT_1_BIT,
        Attributes::PBR::getBindingDescription(),
        Attributes::SHADOWMAP::getAttributeDescriptions(),
        getRenderResource()->m_objectModelIndices,
        GRAPHICS_PIPELINE::SHADOWMAP::DESCRIPTORS_INFO,
        {}
    );
}

template<typename T>
ShadowMap<T>::~ShadowMap() {}


template<typename T>
void ShadowMap<T>::updateUBO() 
{
    for (uint32_t i = 0; i < getRenderResource()->m_objectModelIndices.size(); i++)
    {
        // TODO: Improve this.
        // The model and projection matrix don't need to be updated every frame.
        m_basicInfo.model = std::dynamic_pointer_cast<NormalPBR>(getRenderResource()->m_modelResource[getRenderResource()->m_objectModelIndices[i]])->getModelM();
        glm::mat4 proj = MathUtils::getUpdatedProjMatrix(glm::radians(Config::FOV), 1.0, Config::Z_NEAR_SHADOW, Config::Z_FAR_SHADOW);
        //glm::mat4 proj = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);

        proj[1][1] *= -1;

        glm::fvec4 directionalLightStartPos = std::dynamic_pointer_cast<Light>(getRenderResource()->m_modelResource[getRenderResource()->m_directionalLightIndex])->getPos();
        glm::fvec4 directionalLightEndPos = std::dynamic_pointer_cast<Light>(getRenderResource()->m_modelResource[getRenderResource()->m_directionalLightIndex])->getTargetPos();

        glm::fvec3 lightDir = glm::normalize(glm::fvec3(directionalLightEndPos) - glm::fvec3(directionalLightStartPos));


        //glm::mat4 view = glm::lookAt(glm::fvec3(directionalLightStartPos),glm::fvec3(directionalLightEndPos),glm::fvec3(0.0f, 1.0f, 0.0f));
        glm::mat4 view = glm::lookAt(glm::fvec3(directionalLightStartPos), glm::fvec3(directionalLightStartPos) + lightDir, glm::fvec3(0.0f, 0.0f, 1.0f));

        m_basicInfo.lightSpace = proj * view;


        void* data;
        vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocations[i], &data);
        memcpy(data, &m_basicInfo, sizeof(m_basicInfo));
        vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocations[i]);
    }
}

template<typename T>
const VkCommandBuffer& ShadowMap<T>::getCommandBuffer(const uint32_t index) const
{
    return m_commandBuffers[index];
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
void ShadowMap<T>::bindData(const VkCommandBuffer& commandBuffer, const uint32_t currentFrame)
{
    for (uint32_t i = 0; i < getRenderResource()->m_objectModelIndices.size(); i++)
    {
        auto& model = getRenderResource()->m_modelResource[getRenderResource()->m_objectModelIndices[i]];
        if (model->isHidden())
            continue;

        auto meshes = &(std::dynamic_pointer_cast<NormalPBR>(model)->getMeshes());

        for (auto mesh = meshes->begin(); mesh != meshes->end(); mesh++)
        {
            std::vector<VkBuffer> vertexBuffers = { mesh->vertexBuffer };
            std::vector<VkDeviceSize> offsets = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
            vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);


            const std::vector<VkDescriptorSet> sets = { m_modelDescriptorSets[i]};
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_graphicsPipeline.getPipelineLayout(),
                // Index of the first descriptor set.
                0,
                sets.size(), sets.data(),
                0, {}
            );

            vkCmdDrawIndexed(commandBuffer, mesh->indices.size(), 1, 0, 0, 0);
        }
    }
}

template<typename T>
const VkFramebuffer& ShadowMap<T>::getFramebuffer(const uint32_t imageIndex) const
{
    return m_framebuffers[imageIndex];
}

template<typename T>
const VkCommandPool& ShadowMap<T>::getCommandPool() const
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

    vkDestroyDescriptorPool(getRendererPointer()->getDevice(), m_descriptorPool, nullptr);
    
    m_texture->destroy();

    for (size_t i = 0; i < m_ubos.size(); i++)
        vmaDestroyBuffer(getRendererPointer() ->getVmaAllocator(), m_ubos[i], m_uboAllocations[i]);

    vkDestroyCommandPool(getRendererPointer()->getDevice(), m_commandPool, nullptr);

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
        { shadowMapAttachment },
        { subpass },
        {}
        //dependencies
    );

}


////////////////////////////////////INSTANCES//////////////////////////////////
template class ShadowMap<Attributes::PBR::Vertex>;