#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "VulkanRenderer/Texture/Texture.h"

struct DescriptorInfo
{
    int bindingNumber;
    VkDescriptorType descriptorType;
    VkShaderStageFlagBits shaderStage;
};

////////////////////////////////Helper functions///////////////////////////////
inline static void createDescriptorBufferInfo(const VkBuffer& buffer, VkDescriptorBufferInfo& bufferInfo)
{
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
}

inline static void createDescriptorImageInfo(const VkImageView& imageView, const VkSampler& sampler, VkDescriptorImageInfo& imageInfo)
{
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;
}
///////////////////////////////////////////////////////////////////////////////


namespace DescriptorManager
{
    template<typename T>
    VkResult createDescriptorWriteInfo(
        const T& descriptorInfo,
        const VkDescriptorSet& descriptorSet,
        const uint32_t& dstBinding,
        const uint32_t& dstArrayElement,
        const VkDescriptorType& type,
        VkWriteDescriptorSet& descriptorWrite
    );

    VkResult createDescriptorSet(
        const std::vector<DescriptorInfo>& descriptorInfos,
        const std::vector<std::shared_ptr<TextureBase>>& textures,
        const std::vector<VkDescriptorImageInfo*>& additionalTextureInfos,
        const std::vector<VkBuffer>& UBOs,
        VkDescriptorSet* descriptorset
    );

    VkResult createDescriptorPool(std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPool* descriptorPool);

    VkResult allocDescriptorSet(const VkDescriptorPool& pool,const VkDescriptorSetLayout& layout, VkDescriptorSet* descriptorSet);

};
