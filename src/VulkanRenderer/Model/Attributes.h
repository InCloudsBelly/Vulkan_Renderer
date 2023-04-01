#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VulkanRenderer/RenderDataTypes.h"

namespace Attributes
{
    namespace PBR
    {
        VkVertexInputBindingDescription getBindingDescription();
        std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    namespace SKYBOX
    {
        VkVertexInputBindingDescription getBindingDescription();
        std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    namespace LIGHT
    {
        VkVertexInputBindingDescription getBindingDescription();
        std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };


    namespace SHADOWMAP
    {
        VkVertexInputBindingDescription getBindingDescription();
        std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };
};