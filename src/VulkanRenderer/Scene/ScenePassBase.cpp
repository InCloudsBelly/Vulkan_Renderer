#include "ScenePassBase.h"

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Buffer/BufferManager.h"


void ScenePassBase::createUniformBuffer(const std::shared_ptr<Model> modelPtr, std::vector<size_t>& uboSizeInfos)
{
    for (uint32_t meshIndex : modelPtr->getMeshIndices())
    {
        // create UBO PerMesh
        m_meshesUBOMap[meshIndex].resize(uboSizeInfos.size());
        m_meshesUBOAllocationMap[meshIndex].resize(uboSizeInfos.size());

        for (uint32_t i = 0; i < uboSizeInfos.size(); ++i)
        {
            BufferManager::bufferCreateBuffer(
                getRendererPointer()->getVmaAllocator(),
                uboSizeInfos[i],
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                &m_meshesUBOMap[meshIndex][i],
                &m_meshesUBOAllocationMap[meshIndex][i]
            );
        }
    }
}

void ScenePassBase::drawPipeline(const VkCommandBuffer& commandBuffer, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, std::vector<std::shared_ptr<Model>> models)
{

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Set Dynamic States
    VkViewport viewport{ 0.0f, 0.0f, m_extent.width,m_extent.height, 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{ {0,0}, {m_extent.width,m_extent.height} };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    for (auto& ptr : models)
    {
        if (ptr->isHidden() == false)
        {
            for (uint32_t meshIndex : ptr->getMeshIndices())
            {
                RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];

                std::vector<VkDeviceSize> offsets = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, renderMeshInfo.ref_mesh->vertexBuffer, offsets.data());
                vkCmdBindIndexBuffer(commandBuffer, *renderMeshInfo.ref_mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                const std::vector<VkDescriptorSet> sets = { getMeshDescriptorSet(meshIndex) };
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    0,
                    sets.size(), sets.data(),
                    0, {}
                );

                vkCmdDrawIndexed(commandBuffer, renderMeshInfo.ref_mesh->meshIndexCount, 1, 0, 0, 0);
            }
        }
    }
}

void ScenePassBase::createColorAttachments(std::vector<ColorAttachmentInfo> infos)
{
    m_colorAttachments.resize(infos.size());

    for (int i = 0; i < m_colorAttachments.size(); ++i)
    {
        m_colorAttachments[i] = std::make_shared<NormalTexture>(infos[i].name);
        m_colorAttachments[i]->getExtent() = infos[i].extent;
        m_colorAttachments[i]->getFormat() = infos[i].format;

        BufferManager::bufferCreateOffscreenResources(
            getRendererPointer()->getDevice(),
            getRendererPointer()->getVmaAllocator(),
            getRendererPointer()->getGraphicsQueue(),
            m_colorAttachments[i]->getExtent(),
            m_colorAttachments[i]->getFormat(),
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            1,
            1,
            0,
            infos[i].sampleCount,
            VK_IMAGE_VIEW_TYPE_2D,
            &m_colorAttachments[i]->getAllocation(),
            m_colorAttachments[i]
        );
    }

}
