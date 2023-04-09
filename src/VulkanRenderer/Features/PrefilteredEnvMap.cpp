#include "VulkanRenderer/Features/PrefilteredEnvMap.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Model/Attributes.h"
#include "VulkanRenderer/Framebuffer/FramebufferManager.h"
#include "VulkanRenderer/RenderPass/AttachmentUtils.h"
#include "VulkanRenderer/RenderPass/SubPassUtils.h"
#include "VulkanRenderer/Texture/MipmapUtils.h"
#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Descriptor/DescriptorManager.h"
#include "VulkanRenderer/Pipeline/PipelineManager.h"
#include "VulkanRenderer/Shader/ShaderManager.h"
#include "VulkanRenderer/Renderer.h"


PrefilteredEnvMap::PrefilteredEnvMap( const uint32_t dim) 
    :  m_dim(dim), m_format(VK_FORMAT_R16G16B16A16_SFLOAT)
{
    m_mipLevels = MipmapUtils::getAmountOfSupportedMipLevels(dim, dim);

    createTargetImage();
    createRenderPass();
    createOffscreenFramebuffer( );
    createPipeline();
    createDescriptorSet();
    recordCommandBuffer();
}

void PrefilteredEnvMap::createDescriptorSet()
{
    DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_descriptorSetLayout, &m_descriptorSet);

    VkDescriptorImageInfo SkyboxCubeMap = DescriptorManager::descriptorImageInfo(getRenderResource()->m_skyboxCubeMap->getSampler(), getRenderResource()->m_skyboxCubeMap->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        DescriptorManager::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,&SkyboxCubeMap)
    };
    vkUpdateDescriptorSets(getRendererPointer()->getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

}


void PrefilteredEnvMap::recordCommandBuffer() 
{
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

            VkCommandBuffer commandBuffer = CommandManager::cmdBeginSingleTimeCommands(getRendererPointer()->getDevice(), getRendererPointer()->getCommandPool());

            //---------------------------------CMDs----------------------------
                // Set Dynamic States
            VkViewport viewport{ 0.0f, 0.0f, viewportDim,viewportDim, 0.0f, 1.0f };
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{ {0,0}, {m_dim,m_dim} };
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            //--------------------------RenderPass--------------------------

                // Render scene from cube face's point of view
            m_renderPass.begin(m_framebuffer, { m_dim,m_dim }, { clearValues }, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

            m_pushBlock.mvp = glm::perspective((float)(glm::pi<float>() / 2.0), 1.0f, 0.1f, float(m_dim)) * matrices[face];
            m_pushBlock.roughness = (float)m / float(m_mipLevels - 1);

            vkCmdPushConstants(
                commandBuffer,
                m_pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushBlockPrefilterEnv),
                &m_pushBlock
            );

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

            const std::vector<VkDescriptorSet> sets = { m_descriptorSet };
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pipelineLayout,
                // Index of the first descriptor set.
                0,
                sets.size(), sets.data(),
                0, {}
            );


            {
                RenderMeshInfo& info = getRenderResource()->m_meshInfoMap[getRenderResource()->m_defaultCubeMeshIndex];

                std::vector<VkDeviceSize> offsets = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, info.ref_mesh->vertexBuffer, offsets.data());
                vkCmdBindIndexBuffer(commandBuffer, *info.ref_mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdDrawIndexed(commandBuffer, info.ref_mesh->meshIndexCount, 1, 0, 0, 0);
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

            CommandManager::cmdEndSingleTimeCommands(getRendererPointer()->getDevice(), getRendererPointer()->getGraphicsQueue(), getRendererPointer()->getCommandPool(), commandBuffer);
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



void PrefilteredEnvMap::copyRegionOfImage(
    float face,
    float mipLevel,
    float viewportDim,
    const VkCommandBuffer& commandBuffer
) 
{
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



void PrefilteredEnvMap::createPipeline()
{

    //-------------------------------- PrefilteredEnvMap  Pipeline --------------------------------------
    {
        const std::vector<DescriptorInfo>& descriptorInfo = GRAPHICS_PIPELINE::PREFILTER_ENV_MAP::DESCRIPTORS_INFO;

        std::vector<VkDescriptorSetLayoutBinding> bindings(descriptorInfo.size());
        for (uint32_t i = 0; i < descriptorInfo.size(); i++)
        {
            bindings[i].binding = descriptorInfo[i].bindingNumber;
            bindings[i].descriptorType = descriptorInfo[i].descriptorType;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = descriptorInfo[i].shaderStage;
            bindings[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout!");


        // -------------------Shader Modules--------------------
        const std::vector<ShaderInfo>& shaderInfos = { {shaderType::VERTEX,"prefilterEnvMap"},{shaderType::FRAGMENT,"prefilterEnvMap"} };

        std::vector<VkShaderModule> shaderModules(shaderInfos.size());
        std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos(shaderInfos.size());
        for (uint32_t i = 0; i < shaderInfos.size(); i++)
        {
            PipelineManager::createShaderModule(shaderInfos[i], shaderModules[i]);
            PipelineManager::createShaderStageInfo(shaderModules[i], shaderInfos[i].type, shaderStagesInfos[i]);
        }


        // Dynamic states
        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = PipelineManager::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());


        // Viewport state info
        VkPipelineViewportStateCreateInfo viewportState = PipelineManager::pipelineViewportStateCreateInfo(1, 1, 0);
        // Vertex input(attributes)
        std::vector<VkVertexInputAttributeDescription> attribDescription = Attributes::SKYBOX::getAttributeDescriptions();
        VkVertexInputBindingDescription bindingDescription = Attributes::SKYBOX::getBindingDescription();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        PipelineManager::createVertexShaderInputInfo(bindingDescription, attribDescription, vertexInputInfo);
        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = PipelineManager::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizationState = PipelineManager::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0, VK_FALSE);
        // Multisampling 
        VkPipelineMultisampleStateCreateInfo multisampleState = PipelineManager::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
        // Color blending(attachment)
        VkPipelineColorBlendAttachmentState blendAttachmentState = PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        // Color blending(global)
        VkPipelineColorBlendStateCreateInfo colorBlendState = PipelineManager::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        // Pipeline layout
        std::vector<VkPushConstantRange> pushConstantRanges{ {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockPrefilterEnv)} };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

        auto status = vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        if (status != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");

        // Depth and stencil
        VkPipelineDepthStencilStateCreateInfo depthStencilState = PipelineManager::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        depthStencilState.front = depthStencilState.back;
        depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;


        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderInfos.size();
        pipelineInfo.pStages = shaderStagesInfos.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizationState;
        pipelineInfo.pMultisampleState = &multisampleState;
        pipelineInfo.pDepthStencilState = &depthStencilState;
        pipelineInfo.pColorBlendState = &colorBlendState;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pTessellationState = nullptr;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass.get();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        status = vkCreateGraphicsPipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
        if (status != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline!");

        for (auto& shaderModule : shaderModules)
        {
            ShaderManager::destroyShaderModule(shaderModule);
        }
    }
}



void PrefilteredEnvMap::createOffscreenFramebuffer() 
{

    m_offscreenImage = std::make_shared<NormalTexture>("Offscreen");
    m_offscreenImage->getExtent() = VkExtent2D({ m_dim ,m_dim });
    m_offscreenImage->getFormat() = m_format;

    BufferManager::bufferCreateOffscreenResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
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
        &m_framebuffer
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



void PrefilteredEnvMap::createRenderPass()
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
        { colorAttachment },
        { subPassDescript },
        dependencies
    );
}



void PrefilteredEnvMap::createTargetImage() 
{
    m_targetImage = std::make_shared<CubeMapTexture>("PreEnv");
    m_targetImage->getExtent() = VkExtent2D({ m_dim, m_dim });
    m_targetImage->getFormat() = m_format;

    BufferManager::bufferCreateOffscreenResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
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


PrefilteredEnvMap::~PrefilteredEnvMap() {}


void PrefilteredEnvMap::destroy()
{
    vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayout, nullptr);
    vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayout, nullptr);

    m_targetImage->destroy();
    m_offscreenImage->destroy();
    m_renderPass.destroy();

    vkDestroyFramebuffer( getRendererPointer()->getDevice(), m_framebuffer, nullptr);
}


const std::shared_ptr<TextureBase> PrefilteredEnvMap::get() const
{
    return m_targetImage;
}

