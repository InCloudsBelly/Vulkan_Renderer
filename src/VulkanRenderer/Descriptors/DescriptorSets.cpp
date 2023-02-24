#include "VulkanRenderer/Descriptors/DescriptorSets.h"

#include <vector>
#include <array>

#include <vulkan/vulkan.h>

#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Descriptors/DescriptorTypes/DescriptorTypesUtils.h"
#include "VulkanRenderer/Settings/Config.h"


// Creates, allocates and configures the descriptor sets.
void DescriptorSets::createDescriptorSets(
    const VkDevice logicalDevice,
    const VkImageView& textureImageView,
    const VkSampler& textureSampler,
    std::vector<VkBuffer>& uniformBuffers,
    const VkDescriptorSetLayout& descriptorSetLayout,
    DescriptorPool& descriptorPool )
{
    m_descriptorSets.resize(Config::MAX_FRAMES_IN_FLIGHT);
    m_descriptorSetLayouts.resize(Config::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    descriptorPool.allocDescriptorSets(logicalDevice, m_descriptorSets, m_descriptorSetLayouts);

    // Configures the descriptor sets
    for (size_t i = 0; i < m_descriptorSets.size(); i++)
    {

        VkDescriptorBufferInfo bufferInfo{};
        DescriptorTypesUtils::createDescriptorBufferInfo(uniformBuffers[i], bufferInfo);
        VkDescriptorImageInfo imageInfo{};
        DescriptorTypesUtils::createDescriptorImageInfo(textureImageView, textureSampler, imageInfo);

        // Describes how to update the descriptors.
        // (how and which buffer/image use to bind with the each descriptor)
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        // Another
        createDescriptorWriteInfo(bufferInfo, m_descriptorSets[i], 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorWrites[0]);

        // Another
        createDescriptorWriteInfo(imageInfo, m_descriptorSets[i], 1, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorWrites[1]);

        vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

template<typename T>
void DescriptorSets::createDescriptorWriteInfo(
    const T& descriptorInfo,
    const VkDescriptorSet& descriptorSet,
    const uint32_t& dstBinding,
    const uint32_t& dstArrayElement,
    const VkDescriptorType& type,
    VkWriteDescriptorSet& descriptorWrite)
{
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = dstBinding;
    descriptorWrite.dstArrayElement = dstArrayElement;
    descriptorWrite.descriptorType = type;
    descriptorWrite.descriptorCount = 1;

    if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        descriptorWrite.pBufferInfo = (VkDescriptorBufferInfo*)&descriptorInfo;
    if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        descriptorWrite.pImageInfo = (VkDescriptorImageInfo*)&descriptorInfo;

}

//////////////////////////////////////Instances////////////////////////////////

template void DescriptorSets::createDescriptorWriteInfo<VkDescriptorBufferInfo>(
    const VkDescriptorBufferInfo& descriptorInfo,
    const VkDescriptorSet& descriptorSet,
    const uint32_t& dstBinding,
    const uint32_t& dstArrayElement,
    const VkDescriptorType& type,
    VkWriteDescriptorSet& descriptorWrite
    );

template void DescriptorSets::createDescriptorWriteInfo<VkDescriptorImageInfo>(
    const VkDescriptorImageInfo& descriptorInfo,
    const VkDescriptorSet& descriptorSet,
    const uint32_t& dstBinding,
    const uint32_t& dstArrayElement,
    const VkDescriptorType& type,
    VkWriteDescriptorSet& descriptorWrite
    );


///////////////////////////////////////////////////////////////////////////////

const VkDescriptorSet& DescriptorSets::getDescriptorSet(
    const uint32_t index
) const {
    return m_descriptorSets[index];
}