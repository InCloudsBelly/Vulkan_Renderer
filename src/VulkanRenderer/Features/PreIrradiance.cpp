#include "VulkanRenderer/Features/PreIrradiance.h"

#include <iostream>
#include <vulkan/vulkan.h>

#include <glm/gtc/matrix_transform.hpp>

#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Model/ModelInfo.h"
#include "VulkanRenderer/Model/Attributes.h"
#include "VulkanRenderer/Framebuffer/FramebufferManager.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
#include "VulkanRenderer/Texture/MipmapUtils.h"
#include "VulkanRenderer/Descriptor/DescriptorPool.h"
#include "VulkanRenderer/Command/CommandManager.h"

#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Renderer.h"

template<typename T>
PrefilteredIrradiance<T>::PrefilteredIrradiance(
    const VkQueue& graphicsQueue,
    const std::shared_ptr<CommandPool>& commandPool,
    const uint32_t dim,
    const std::vector<Mesh<T>>& meshes,
    const std::shared_ptr<TextureBase>& envMap
) : m_dim(dim), m_format(VK_FORMAT_R32G32B32A32_SFLOAT)
{
    m_mipLevels = MipmapUtils::getAmountOfSupportedMipLevels(dim, dim);

    createTargetImage();
    createRenderPass();
    createOffscreenFramebuffer(graphicsQueue, commandPool);
    createPipeline();
    createDescriptorPool();
    createDescriptorSet(envMap);
    recordCommandBuffer(commandPool, graphicsQueue, meshes);
}

template<typename T>
void PrefilteredIrradiance<T>::createDescriptorPool()
{
    m_descriptorPool = DescriptorPool(
         getRendererPointer()->getDevice(),
        {
           {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2 }
        },
        2
    );
}

template<typename T>
void PrefilteredIrradiance<T>::createDescriptorSet(const std::shared_ptr<TextureBase>& envMap)
{
    m_descriptorSets = DescriptorSets(
         getRendererPointer()->getDevice(),
        {},
        GRAPHICS_PIPELINE::PREFILTER_ENV_MAP::SAMPLERS_INFO,
        { envMap },
        m_graphicsPipeline.getDescriptorSetLayout(),
        m_descriptorPool
    );
}

template<typename T>
void PrefilteredIrradiance<T>::recordCommandBuffer(
    const std::shared_ptr<CommandPool>& commandPool,
    const VkQueue& graphicsQueue,
    const std::vector<Mesh<T>>& meshes
) {
    VkClearValue clearValues;
    clearValues.color = { {0.0f, 0.0f, 0.2f, 0.0f} };

    std::vector<glm::mat4> matrices = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,   0.0f,  0.0f), glm::vec3(0.0f,  -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f,  -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,   1.0f,  0.0f), glm::vec3(0.0f,  0.0f,   1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,   0.0f,  1.0f), glm::vec3(0.0f,  -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,   0.0f, -1.0f), glm::vec3(0.0f,  -1.0f,  0.0f))
    };


    const VkCommandBuffer& commandBuffer = commandPool->getCommandBuffer(0);


    BufferManager::bufferTransitionImageLayout(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(),
        m_targetImage->getImage(),
        m_targetImage->getFormat(),
        VK_IMAGE_ASPECT_COLOR_BIT,
        m_mipLevels,
        6,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );


    for (uint32_t m = 0; m < m_mipLevels; m++)
    {
        for (uint32_t face = 0; face < 6; face++)
        {
            //float viewportDim = static_cast<float>(m_dim * std::pow(0.5f, m));
            uint32_t viewportDim = static_cast<uint32_t>(m_dim * std::pow(0.5f, m));
            commandPool->resetCommandBuffer(0);
            commandPool->beginCommandBuffer(0, commandBuffer);

            //---------------------------------CMDs----------------------------
                // Set Dynamic States
            CommandManager::STATE::setViewport(0.0f, 0.0f, { static_cast<uint32_t>(viewportDim), static_cast<uint32_t>(viewportDim), }, 0.0f, 1.0f, 0, 1, commandBuffer);
            CommandManager::STATE::setScissor({ 0, 0 }, { m_dim,m_dim }, 0, 1, commandBuffer);

            //--------------------------RenderPass--------------------------

                // Render scene from cube face's point of view
            m_renderPass.begin(m_framebuffer, { m_dim,m_dim }, { clearValues }, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

            m_pushBlock.mvp = glm::perspective((float)(glm::pi<float>() / 2.0), 1.0f, 0.1f, 512.0f) * matrices[face];

            vkCmdPushConstants(
                commandBuffer,
                m_graphicsPipeline.getPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushBlockIrradiance),
                &m_pushBlock
            );

            CommandManager::STATE::bindPipeline(m_graphicsPipeline.get(), PipelineType::GRAPHICS, commandBuffer);

            CommandManager::STATE::bindDescriptorSets(
                m_graphicsPipeline.getPipelineLayout(),
                PipelineType::GRAPHICS,
                // Index of first descriptor set.
                0,
                { m_descriptorSets.get(0) },
                // Dynamic offsets.
                {},
                commandBuffer
            );

            for (auto& mesh : meshes)
            {
                CommandManager::STATE::bindVertexBuffers(
                    { mesh.vertexBuffer },
                    // Offsets.
                    { 0 },
                    // Index of first binding.
                    0,
                    // Bindings count.
                    1,
                    commandBuffer
                );
                CommandManager::STATE::bindIndexBuffer(
                    mesh.indexBuffer,
                    // Offset.
                    0,
                    VK_INDEX_TYPE_UINT32,
                    commandBuffer
                );

                CommandManager::ACTION::drawIndexed(
                    // Index Count
                    mesh.indices.size(),
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

            m_renderPass.end(commandBuffer);

            {
                BufferManager::bufferTransitionImageLayout(
                    getRendererPointer()->getDevice(),
                    getRendererPointer()->getGraphicsQueue(),
                    commandBuffer,
                    m_offscreenImage->getImage(),
                    m_offscreenImage->getFormat(),
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    1,
                    1,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                );
            }

            copyRegionOfImage(face, m, viewportDim, commandBuffer);

            {
                BufferManager::bufferTransitionImageLayout(
                    getRendererPointer()->getDevice(),
                    getRendererPointer()->getGraphicsQueue(),
                    commandBuffer,
                    m_offscreenImage->getImage(),
                    m_offscreenImage->getFormat(),
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    1,
                    1,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                );
            }

            commandPool->endCommandBuffer(commandBuffer);
            commandPool->submitCommandBuffer(graphicsQueue, { commandBuffer }, true, {}, std::nullopt, {}, std::nullopt);
        }
    }



    BufferManager::bufferTransitionImageLayout(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(),
        m_targetImage->getImage(),
        m_targetImage->getFormat(),
        VK_IMAGE_ASPECT_COLOR_BIT,
        m_mipLevels,
        6,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}


template<typename T>
void PrefilteredIrradiance<T>::copyRegionOfImage(
    float face,
    float mipLevel,
    float viewportDim,
    const VkCommandBuffer& commandBuffer
) {
    VkImageCopy copyRegion{};

    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = { 0, 0, 0 };

    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.baseArrayLayer = face;
    copyRegion.dstSubresource.mipLevel = mipLevel;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset = { 0, 0, 0 };

    copyRegion.extent.width = static_cast<uint32_t>(viewportDim);
    copyRegion.extent.height = static_cast<uint32_t>(viewportDim);
    copyRegion.extent.depth = 1;

    vkCmdCopyImage(
        commandBuffer,
        m_offscreenImage->getImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        m_targetImage->getImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion
    );
}


template<typename T>
void PrefilteredIrradiance<T>::createPipeline()
{
    m_graphicsPipeline = Graphics(
         getRendererPointer()->getDevice(),
        GraphicsPipelineType::PREFILTER_ENV_MAP,
        { m_dim,m_dim },
        m_renderPass,
        { {shaderType::VERTEX,"preIrradiance"},{shaderType::FRAGMENT,"preIrradiance"} },
        VK_SAMPLE_COUNT_1_BIT,
        // It uses the same attributes as the skybox shader.
        Attributes::SKYBOX::getBindingDescription(),
        Attributes::SKYBOX::getAttributeDescriptions(),
        {},
        {},
        GRAPHICS_PIPELINE::PREFILTER_IRRADIANCE::SAMPLERS_INFO,
        { {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(PushBlockIrradiance)} }
    );
}


template<typename T>
void PrefilteredIrradiance<T>::createOffscreenFramebuffer(
    const VkQueue& graphicsQueue,
    const std::shared_ptr<CommandPool>& commandPool
) {
    
    m_offscreenImage = std::make_shared<NormalTexture>("Offscreen");
    m_offscreenImage->getExtent() = VkExtent2D({ m_dim ,m_dim });
    m_offscreenImage->getFormat() = m_format;

    BufferManager::bufferCreateOffscreenResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(), 
        m_offscreenImage->getExtent(),
        m_offscreenImage->getFormat(),
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        1,
        1,
        0,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_VIEW_TYPE_2D,
        &m_offscreenImage->getAllocation(),
        m_offscreenImage
    );

    BufferManager::bufferTransitionImageLayout(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(),
        m_offscreenImage->getImage(),
        m_offscreenImage->getFormat(),
        VK_IMAGE_ASPECT_COLOR_BIT,
        1,
        1,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );


    std::vector<VkImageView> attachments = { m_offscreenImage->getImageView() };

    FramebufferManager::createFramebuffer(
         getRendererPointer()->getDevice(),
        m_renderPass.get(),
        attachments,
        m_dim,
        m_dim,
        1,
        m_framebuffer
    );


    BufferManager::bufferTransitionImageLayout(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(),
        m_offscreenImage->getImage(),
        m_offscreenImage->getFormat(),
        VK_IMAGE_ASPECT_COLOR_BIT,
        1,
        1,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
}


template<typename T>
void PrefilteredIrradiance<T>::createRenderPass()
{
    // Color Attachment
    VkAttachmentDescription colorAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        m_format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorAttachment
    );

    // Attachment references

    VkAttachmentReference colorAttachmentRef{};
    AttachmentUtils::createAttachmentReference(
        0,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorAttachmentRef
    );

    // Subpasses
    VkSubpassDescription subPassDescript{};
    SubPassUtils::createSubPassDescription(
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        &colorAttachmentRef,
        nullptr,
        nullptr,
        subPassDescript
    );

    // Subpass dependencies
    std::vector<VkSubpassDependency> dependencies(2);
    SubPassUtils::createSubPassDependency(
        // -Source parameters.
        //VK_SUBPASS_EXTERNAL means anything outside of a given render pass
        //scope. When used for srcSubpass it specifies anything that happened 
        //before the render pass. 
        VK_SUBPASS_EXTERNAL,
        // Operations that the subpass needs to wait on. 
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        // -Destination parameters.
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        (
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
            ),
        VK_DEPENDENCY_BY_REGION_BIT,
        dependencies[0]
    );
    SubPassUtils::createSubPassDependency(
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        (
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
            ),
        // -Destination parameters.
        VK_SUBPASS_EXTERNAL,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        dependencies[1]
    );

    m_renderPass = RenderPass(
         getRendererPointer()->getDevice(),
        { colorAttachment },
        { subPassDescript },
        dependencies
    );
}


template<typename T>
void PrefilteredIrradiance<T>::createTargetImage()
{
    m_targetImage = std::make_shared<CubeMapTexture>("Irradiance");
    m_targetImage->getExtent() = VkExtent2D({ m_dim, m_dim });
    m_targetImage->getFormat() = m_format;

    BufferManager::bufferCreateOffscreenResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(),
        m_targetImage->getExtent(),
        m_targetImage->getFormat(),
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        floor(log2(m_dim)) + 1,
        6,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_VIEW_TYPE_CUBE,
        &m_targetImage->getAllocation(),
        m_targetImage
    );


}

template<typename T>
PrefilteredIrradiance<T>::~PrefilteredIrradiance() {}

template<typename T>
void PrefilteredIrradiance<T>::destroy()
{
    m_graphicsPipeline.destroy();
    m_descriptorPool.destroy();
    m_targetImage->destroy();
    m_offscreenImage->destroy();
    m_renderPass.destroy();

    vkDestroyFramebuffer( getRendererPointer()->getDevice(), m_framebuffer, nullptr);
}

template<typename T>
const std::shared_ptr<TextureBase> PrefilteredIrradiance<T>::get() const
{
    return m_targetImage;
}


////////////////////////////////////INSTANCES//////////////////////////////////
template class PrefilteredIrradiance<Attributes::SKYBOX::Vertex>;