#include "VulkanRenderer/Descriptors/DescriptorSets.h"

#include <vector>
#include <iostream>
#include <array>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include "VulkanRenderer/Descriptors/DescriptorInfo.h"
#include "VulkanRenderer/Descriptors/DescriptorPool.h"
#include "VulkanRenderer/Descriptors/Types/DescriptorTypesUtils.h"
#include "VulkanRenderer/Features/ShadowMap.h"
#include "VulkanRenderer/Settings/Config.h"


DescriptorSets::DescriptorSets() {}

// Creates, allocates and configures the descriptor sets.
DescriptorSets::DescriptorSets(
    const VkDevice                              logicalDevice,
    const std::vector<DescriptorInfo>&          uboInfo,
    const std::vector<DescriptorInfo>&          samplersInfo,
    const std::vector<std::shared_ptr<Texture>>& textures,
    std::vector<UBO*>&                          UBOs,
    const VkDescriptorSetLayout&                descriptorSetLayout,
    DescriptorPool&                             descriptorPool,
    const std::optional<Texture>                irradianceMap,
    const std::optional<VkImageView>            shadowMapView,
    const std::optional<VkSampler>              shadowMapSampler)
{
    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.resize(samplersInfo.size());

    m_descriptorSets.resize(Config::MAX_FRAMES_IN_FLIGHT);
    // We can't optimize this since Vulkan hasn't an optimization/function to
    // create descriptors sets with one same Descriptor Set Layout.
    m_descriptorSetLayouts.resize(Config::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    descriptorPool.allocDescriptorSets(m_descriptorSetLayouts,m_descriptorSets);

    // Configures the descriptor sets
    for (size_t i = 0; i < m_descriptorSets.size(); i++)
    {
        std::vector<VkDescriptorBufferInfo> bufferInfos(UBOs.size());
        for (size_t j = 0; j < UBOs.size(); ++j)
        {
            DescriptorTypesUtils::createDescriptorBufferInfo(UBOs[j]->get(i), bufferInfos[j]);
        }

        // Samplers of textures
        for (size_t j = 0; j < textures.size(); j++)
        {
            DescriptorTypesUtils::createDescriptorImageInfo(textures[j]->getImageView(),textures[j]->getSampler(), imageInfos[j]);
        }

        if (irradianceMap.has_value())
        {
            // Sampler of Irradiance map.
            DescriptorTypesUtils::createDescriptorImageInfo(irradianceMap->getImageView(),irradianceMap->getSampler(),imageInfos[samplersInfo.size() - 2]);
        }

        if (shadowMapView.has_value() && shadowMapSampler.has_value())
        {
            DescriptorTypesUtils::createDescriptorImageInfo(shadowMapView.value(),shadowMapSampler.value(),imageInfos[samplersInfo.size() - 1]);
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.resize(uboInfo.size() + samplersInfo.size());

        // UBOs
        for (size_t j = 0; j < uboInfo.size(); j++)
        {
            createDescriptorWriteInfo(bufferInfos[j], m_descriptorSets[i], uboInfo[j].bindingNumber, 0, uboInfo[j].descriptorType, descriptorWrites[j]);
        }

        //Samplers
        for (size_t j = 0; j < samplersInfo.size(); j++)
        {
            createDescriptorWriteInfo(imageInfos[j], m_descriptorSets[i], samplersInfo[j].bindingNumber, 0, samplersInfo[j].descriptorType, descriptorWrites[uboInfo.size() + j]);
        }

        vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

DescriptorSets::DescriptorSets(const DescriptorSets& other)
    : m_descriptorSets(other.m_descriptorSets),
    m_descriptorSetLayouts(other.m_descriptorSetLayouts)
{}

DescriptorSets& DescriptorSets::operator=(const DescriptorSets& other)
{
    if (this == &other)
        return *this;

    m_descriptorSets = other.m_descriptorSets;
    m_descriptorSetLayouts = other.m_descriptorSetLayouts;

    return *this;
}


DescriptorSets::~DescriptorSets() {}


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