#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace SubPassUtils
{
    void createSubPassDescription(
        const VkPipelineBindPoint& pipelineBindPoint,
        const VkAttachmentReference* colorAttachRef,
        const VkAttachmentReference* depthStencilAttachRef,
        const VkAttachmentReference* colorResolveAttachmentRef,
        VkSubpassDescription& subPassDescription
    );

    void createSubPassDependency(
        const uint32_t& srcSubPass,
        const VkPipelineStageFlags& srcStageFlags,
        const VkAccessFlags& srcAccessMask,
        const uint32_t& dstSubPass,
        const VkPipelineStageFlags& dstStageFlags,
        const VkAccessFlags& dstAccessMask,
        const VkDependencyFlagBits& dependencyFlags,
        VkSubpassDependency& dependency
    );

};