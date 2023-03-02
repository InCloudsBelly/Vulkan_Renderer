#include "VulkanRenderer/Descriptors/Types/DescriptorTypesUtils.h"

#include <vulkan/vulkan.h>

void DescriptorTypesUtils::createDescriptorBufferInfo(
    const VkBuffer& buffer,
    VkDescriptorBufferInfo& bufferInfo
) {
    // Specifies the buffer and the region within it that contains the data
    // for the descriptor set.
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
}

void DescriptorTypesUtils::createDescriptorImageInfo(
    const VkImageView& imageView,
    const VkSampler& sampler,
    VkDescriptorImageInfo& imageInfo
) {
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;
}