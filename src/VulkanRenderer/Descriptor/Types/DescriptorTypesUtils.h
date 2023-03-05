#pragma once

#include <vulkan/vulkan.h>

namespace DescriptorTypesUtils {

    void createDescriptorBufferInfo(
        const VkBuffer& buffer,
        VkDescriptorBufferInfo& bufferInfo
    );
    void createDescriptorImageInfo(
        const VkImageView& imageView,
        const VkSampler& sampler,
        VkDescriptorImageInfo& imageInfo
    );

};