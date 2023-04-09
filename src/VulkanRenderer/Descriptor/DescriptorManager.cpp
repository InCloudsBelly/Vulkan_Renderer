
#include "VulkanRenderer/Descriptor/DescriptorManager.h"

#include "VulkanRenderer/Renderer.h"

template<typename T>
VkResult DescriptorManager::createDescriptorWriteInfo(
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

    if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
    {
        descriptorWrite.pBufferInfo = (VkDescriptorBufferInfo*)&descriptorInfo;
    }
    else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
    {
        descriptorWrite.pImageInfo = (VkDescriptorImageInfo*)&descriptorInfo;
    }

    return VK_SUCCESS;
}




VkResult DescriptorManager::createDescriptorPool(std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPool* descriptorPool)
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    uint32_t maxSets = 0;
    for (auto info : poolSizes)
        maxSets += info.descriptorCount;

    poolInfo.maxSets = maxSets;

    return vkCreateDescriptorPool(getRendererPointer()->getDevice(), &poolInfo, nullptr, descriptorPool);
}

VkResult DescriptorManager::allocDescriptorSet(const VkDescriptorPool& pool, const VkDescriptorSetLayout& layout, VkDescriptorSet* descriptorSet)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    return vkAllocateDescriptorSets(getRendererPointer()->getDevice(), &allocInfo, descriptorSet);
}