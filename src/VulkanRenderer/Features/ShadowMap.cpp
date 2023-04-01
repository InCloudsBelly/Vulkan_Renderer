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

#include "VulkanRenderer/Model/ModelManager.h"
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

    createRenderPass(format);
    createFramebuffer(imagesCount);
    createGraphicsPipeline(extent);
    
    //create DescriptorPool
    std::vector<VkDescriptorPoolSize> poolSizes = {
       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(uint32_t)getRenderResource()->m_meshInfoMap.size() * GRAPHICS_PIPELINE::SHADOWMAP::UBOS_COUNT}
    };
    DescriptorManager::createDescriptorPool(poolSizes, &m_descriptorPool);


    //Create UBOs && DescriptorSets PerMesh
    for (auto ptr : getRenderResource()->m_normalModels)
    {

        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            BufferManager::bufferCreateBuffer(
                getRendererPointer()->getVmaAllocator(),
                sizeof(DescriptorTypes::UniformBufferObject::ShadowMap),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                &m_ubosMap[meshIndex],
                &m_uboAllocationsMap[meshIndex]
            );


            //create DescriptorSet PerMesh
            DescriptorManager::allocDescriptorSet(m_descriptorPool, m_graphicsPipeline.getDescriptorSetLayout(), &m_descriptorSetsMap[meshIndex]);

            DescriptorManager::createDescriptorSet(
                GRAPHICS_PIPELINE::SHADOWMAP::DESCRIPTORS_INFO,
                {}, {},
                { m_ubosMap[meshIndex] },
                &m_descriptorSetsMap[meshIndex]
            );
        }
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
        GRAPHICS_PIPELINE::SHADOWMAP::DESCRIPTORS_INFO,
        {}
    );
}

template<typename T>
ShadowMap<T>::~ShadowMap() {}


template<typename T>
void ShadowMap<T>::updateUBO() 
{
    for (auto ptr : getRenderResource()->m_normalModels)
    {

        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            m_basicInfo.model = ptr->getModelMatrix();

            glm::mat4 proj = MathUtils::getUpdatedProjMatrix(glm::radians(Config::FOV), 1.0, Config::Z_NEAR_SHADOW, Config::Z_FAR_SHADOW);
            //glm::mat4 proj = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);

            proj[1][1] *= -1;

            LightInfo& info = getRenderResource()->m_lightsInfo[getRenderResource()->m_directionalLightIndex];

            glm::fvec3 lightDir = glm::normalize(glm::fvec3(info.m_targetPos) - glm::fvec3(info.pos));


            //glm::mat4 view = glm::lookAt(glm::fvec3(directionalLightStartPos),glm::fvec3(directionalLightEndPos),glm::fvec3(0.0f, 1.0f, 0.0f));
            glm::mat4 view = glm::lookAt(glm::fvec3(info.pos), glm::fvec3(info.pos) + lightDir, glm::fvec3(0.0f, 0.0f, 1.0f));

            m_basicInfo.lightSpace = proj * view;


            void* data;
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex], &data);
            memcpy(data, &m_basicInfo, sizeof(m_basicInfo));
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex]);
        }
    
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

    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            RenderMeshInfo& meshInfo = getRenderResource()->m_meshInfoMap[meshIndex];

            std::vector<VkDeviceSize> offsets = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, meshInfo.ref_mesh->vertexBuffer, offsets.data());
            vkCmdBindIndexBuffer(commandBuffer, *meshInfo.ref_mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);


            const std::vector<VkDescriptorSet> sets = { m_descriptorSetsMap[meshIndex] };
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_graphicsPipeline.getPipelineLayout(),
                // Index of the first descriptor set.
                0,
                sets.size(), sets.data(),
                0, {}
            );

            vkCmdDrawIndexed(commandBuffer, meshInfo.ref_mesh->meshIndexCount, 1, 0, 0, 0);
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

    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
             vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_ubosMap[meshIndex], m_uboAllocationsMap[meshIndex]);
        }
    }

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
template class ShadowMap<MeshVertex>;