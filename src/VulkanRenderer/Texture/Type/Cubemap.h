#pragma once

#include "VulkanRenderer/Texture/Texture.h"

#include <string>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/Descriptor/Types/Sampler/Sampler.h"
#include "VulkanRenderer/Image/Image.h"


class Cubemap : public Texture
{

public:

    Cubemap(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        const TextureToLoadInfo& textureInfo,
        const std::string& textureFolderName,
        const VkSampleCountFlagBits& samplesCount,
        const std::shared_ptr<CommandPool>& commandPool,
        VkQueue& graphicsQueue,
        const UsageType& usage = UsageType::TO_COLOR
    );
    ~Cubemap() override;

private:

};