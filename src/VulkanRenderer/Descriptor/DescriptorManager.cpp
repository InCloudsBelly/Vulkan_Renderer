
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


VkResult DescriptorManager::createDescriptorSet(
    const std::vector<DescriptorInfo>& descriptorInfos,
    const std::vector<std::shared_ptr<TextureBase>>& textures,
    const std::vector<VkDescriptorImageInfo*>& additionalTextureInfos,
    const std::vector<VkBuffer>& UBOs,
    VkDescriptorSet* descriptorset
)
{
    std::vector<VkDescriptorBufferInfo> bufferInfos(UBOs.size());
    std::vector<VkDescriptorImageInfo> imageInfos(descriptorInfos.size() - UBOs.size());

    // UBOs  / Samplers of textures / Sampler of additional textures 
    {
        for (uint32_t j = 0; j < bufferInfos.size(); ++j)
            createDescriptorBufferInfo(UBOs[j], bufferInfos[j]);
        for (uint32_t j = 0; j < textures.size(); j++)
            createDescriptorImageInfo(textures[j]->getImageView(), textures[j]->getSampler(), imageInfos[j]);
        for (uint32_t j = 0; j < additionalTextureInfos.size(); j++)
            createDescriptorImageInfo(additionalTextureInfos[j]->imageView, additionalTextureInfos[j]->sampler, imageInfos[textures.size() + j]);
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorInfos.size());

    // UBOs  / Samplers
    {
        for (uint32_t j = 0; j < bufferInfos.size(); j++)
            createDescriptorWriteInfo(bufferInfos[j], *descriptorset, descriptorInfos[j].bindingNumber, 0, descriptorInfos[j].descriptorType, descriptorWrites[j]);
        for (uint32_t j = 0; j < imageInfos.size(); j++)
            createDescriptorWriteInfo(imageInfos[j], *descriptorset, descriptorInfos[bufferInfos.size() + j].bindingNumber, 0, descriptorInfos[bufferInfos.size() + j].descriptorType, descriptorWrites[bufferInfos.size() + j]);
    }

    vkUpdateDescriptorSets(getRendererPointer()->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

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