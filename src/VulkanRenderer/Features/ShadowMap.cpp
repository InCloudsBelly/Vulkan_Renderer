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


ShadowMap::ShadowMap(
    const VkExtent2D& extent,
    const uint32_t imagesCount,
    const VkFormat& format,
    const uint32_t& uboCount
) : m_width(extent.width), m_height(extent.height)
{
    m_clearValuesShadowMap.resize(2);
    m_clearValuesShadowMap[0].depthStencil.depth = 1.0f;
    m_clearValuesShadowMap[1].depthStencil.depth = 1.0f;
    m_clearValuesShadowMap[0].depthStencil.stencil = 0.0f;
    m_clearValuesShadowMap[1].depthStencil.stencil = 0.0f;

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
    
    //Create UBOs && DescriptorSets PerMesh
    createUBOs();
    createDescriptorSets();
   

    //Create CommandPool
    CommandManager::cmdCreateCommandPool(
        getRendererPointer()->getDevice(),
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        getRendererPointer()->getQueueFamilyIndices().graphicsFamily.value(),
        &m_commandPool
    );

    //Create CommandBuffers
    m_commandBuffers.resize(Config::MAX_FRAMES_IN_FLIGHT);
    CommandManager::cmdCreateCommandBuffers(
        getRendererPointer()->getDevice(),
        m_commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        Config::MAX_FRAMES_IN_FLIGHT,
        &m_commandBuffers[0]
    );

}


void ShadowMap::createGraphicsPipeline(const VkExtent2D& extent)
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


ShadowMap::~ShadowMap() {}



void ShadowMap::updateUBO() 
{
    for (auto ptr : getRenderResource()->m_normalModels)
    {

        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            m_basicInfo.model = ptr->getModelMatrix();

            //glm::mat4 proj = MathUtils::getUpdatedProjMatrix(glm::radians(Config::FOV), 1.0, Config::Z_NEAR_SHADOW, Config::Z_FAR_SHADOW);
            glm::mat4 proj = glm::ortho(-8.0f, 8.0f, -8.0f, 8.0f, 0.5f, 50.0f);

            proj[1][1] *= -1;

            LightInfo& info = getRenderResource()->m_lightsInfo[getRenderResource()->m_directionalLightIndex];

            glm::fvec3 lightDir = glm::normalize(info.m_targetPos - info.pos);

            glm::mat4 view = glm::lookAt(info.pos,info.pos + lightDir, glm::fvec3(0.0f, 1.0f, 0.0f));

            m_basicInfo.lightSpace = proj * view;

            void* data;
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex], &data);
            memcpy(data, &m_basicInfo, sizeof(m_basicInfo));
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex]);
        }
    
    }
}

void ShadowMap::draw(uint32_t imageIndex, uint32_t currentFrame)
{
    VkCommandBuffer& commandBuffer = getRendererPointer()->getGraphicsCommandBuffer(currentFrame);

    //--------------------------------RenderPass-----------------------------
    m_renderPass.begin(m_framebuffers[imageIndex], VkExtent2D({ m_width,m_height }), m_clearValuesShadowMap, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline.get());

    // Set Dynamic States
    VkViewport viewport{ 0.0f, 0.0f,m_width,m_height, 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{ {0,0}, {m_width,m_height} };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
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
                    0,
                    sets.size(), sets.data(),
                    0, {}
                );

                vkCmdDrawIndexed(commandBuffer, meshInfo.ref_mesh->meshIndexCount, 1, 0, 0, 0);
            }
        }
    }
    m_renderPass.end(commandBuffer);
}



const VkCommandBuffer& ShadowMap::getCommandBuffer(const uint32_t index) const
{
    return m_commandBuffers[index];
}

const std::shared_ptr<TextureBase> ShadowMap::get() const
{
    return m_texture;
}


const VkImageView& ShadowMap::getShadowMapView() const
{
    return m_texture->getImageView();
}


const VkSampler& ShadowMap::getSampler() const
{
    return m_texture->getSampler();
}

const VkFramebuffer& ShadowMap::getFramebuffer(const uint32_t imageIndex) const
{
    return m_framebuffers[imageIndex];
}


const VkCommandPool& ShadowMap::getCommandPool() const
{
    return m_commandPool;
}


const glm::mat4& ShadowMap::getLightSpace() const
{
    return m_basicInfo.lightSpace;
}


const RenderPass& ShadowMap::getRenderPass() const
{
    return m_renderPass;
}


const Graphics& ShadowMap::getGraphicsPipeline() const
{
    return m_graphicsPipeline;
}


void ShadowMap::createFramebuffer(const uint32_t& imagesCount)
{

    m_framebuffers.resize(imagesCount);
    std::vector<VkImageView> attachments = { m_texture->getImageView() };

    for (uint32_t i = 0; i < imagesCount; i++)
    {
        FramebufferManager::createFramebuffer(getRendererPointer()->getDevice(), m_renderPass.get(), attachments, m_width, m_height, 1, &m_framebuffers[i]);
    }
}

void ShadowMap::createUBOs()
{
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
        }
    }
}

void ShadowMap::createDescriptorSets()
{
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            //create DescriptorSet PerMesh
            DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_graphicsPipeline.getDescriptorSetLayout(), &m_descriptorSetsMap[meshIndex]);

            DescriptorManager::createDescriptorSet(
                GRAPHICS_PIPELINE::SHADOWMAP::DESCRIPTORS_INFO,
                {}, {},
                { m_ubosMap[meshIndex] },
                &m_descriptorSetsMap[meshIndex]
            );
        }
    }
}




void ShadowMap::destroy()
{
    m_graphicsPipeline.destroy();
    
    m_texture->destroy();


    for (auto& uboInfo : m_ubosMap)
    {
        vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), uboInfo.second, m_uboAllocationsMap[uboInfo.first]);
    }


    vkDestroyCommandPool(getRendererPointer()->getDevice(), m_commandPool, nullptr);

    m_renderPass.destroy();

    for (auto& framebuffer : m_framebuffers)
        vkDestroyFramebuffer(getRendererPointer()->getDevice(), framebuffer, nullptr);
}


void ShadowMap::createRenderPass(const VkFormat& depthBufferFormat)
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
