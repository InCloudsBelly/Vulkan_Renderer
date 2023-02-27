#include "VulkanRenderer/Descriptors/DescriptorSets.h"

#include <vector>
#include <array>

#include <vulkan/vulkan.h>
#include "VulkanRenderer/Descriptors/DescriptorInfo.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Descriptors/Types/DescriptorTypesUtils.h"
#include "VulkanRenderer/Settings/Config.h"


// Creates, allocates and configures the descriptor sets.
void DescriptorSets::createDescriptorSets(
    const VkDevice                      logicalDevice,
    const std::vector<DescriptorInfo>&  uboInfo,
    const std::vector<DescriptorInfo>&  samplersInfo,
    const std::vector<Texture>&         textures,
    std::vector<VkBuffer>&              uniformBuffers,
    const VkDescriptorSetLayout&        descriptorSetLayout,
    DescriptorPool&                     descriptorPool )
{
    // TODO: Improve this, since all the descriptors sets
    // are the same, just copy it instead of creating them
    // many times.

    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.resize(textures.size());

    m_descriptorSets.resize(Config::MAX_FRAMES_IN_FLIGHT);
    m_descriptorSetLayouts.resize(Config::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    descriptorPool.allocDescriptorSets(logicalDevice, m_descriptorSets, m_descriptorSetLayouts);

    // Configures the descriptor sets
    for (size_t i = 0; i < m_descriptorSets.size(); i++)
    {

        VkDescriptorBufferInfo bufferInfo{};
        DescriptorTypesUtils::createDescriptorBufferInfo(uniformBuffers[i], bufferInfo);

        for (size_t j = 0; j < imageInfos.size(); j++)
        {
            DescriptorTypesUtils::createDescriptorImageInfo(textures[j].getTextureImageView(), textures[j].getTextureSampler(), imageInfos[j]);
        }
        
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.resize(uboInfo.size() + samplersInfo.size());

        // UBOs
        for (size_t j = 0; j < uboInfo.size(); j++)
        {
            createDescriptorWriteInfo(bufferInfo,m_descriptorSets[i],uboInfo[j].bindingNumber,0,uboInfo[j].descriptorType,descriptorWrites[j]);
        }

        // Samplers
        for (size_t j = 0; j < samplersInfo.size(); j++)
        {
            createDescriptorWriteInfo(imageInfos[j],m_descriptorSets[i],samplersInfo[j].bindingNumber,0,samplersInfo[j].descriptorType,descriptorWrites[uboInfo.size() + j]);
        }


        vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

template<typename T>
void DescriptorSets::createDescriptorWriteInfo(
    const T&                descriptorInfo,
    const VkDescriptorSet&  descriptorSet,
    const uint32_t&         dstBinding,
    const uint32_t&         dstArrayElement,
    const VkDescriptorType& type,
    VkWriteDescriptorSet&   descriptorWrite)
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

const VkDescriptorSet& DescriptorSets::get(const uint32_t index) const 
{
    return m_descriptorSets[index];
}