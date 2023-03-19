#include "VulkanRenderer/Texture/Texture.h"

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <iostream>

#include <VMA/vk_mem_alloc.h>

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Texture/MipmapUtils.h"

#define CHECKRESULT(x) { \
        VkResult retval = (x); \
        assert (retval == VK_SUCCESS); \
    }

void TextureBase::destroy()
{
    if (m_imageInfo.sampler != VK_NULL_HANDLE)
        vkDestroySampler(getRendererPointer()->getDevice(), m_imageInfo.sampler, nullptr);
    if (m_imageInfo.imageView != VK_NULL_HANDLE)
        vkDestroyImageView(getRendererPointer()->getDevice(), m_imageInfo.imageView, nullptr);
    if (m_image != VK_NULL_HANDLE)
        vmaDestroyImage(getRendererPointer()->getVmaAllocator(), m_image, m_deviceAllocation);
}


NormalTexture::NormalTexture(
    const std::string name,
    const std::string& basedir,
    const VkFormat& format,
    const VkImageCreateFlags flags,
    const VkImageViewType viewType
) : TextureBase(name, format, flags, viewType)
{

    CHECKRESULT(
        BufferManager::bufferCreateTextureImage(
            getRendererPointer()->getDevice(),
            getRendererPointer()->getVmaAllocator(),
            getRendererPointer()->getGraphicsQueue(),
            getRendererPointer()->getCommandPool(),
            basedir,
            name,
            m_format, 
            flags, 
            &m_image,
            &m_deviceAllocation, 
            &m_extent,
            &m_mipmapLevel)
    );

    CHECKRESULT(
        BufferManager::bufferCreateImageView(
            getRendererPointer()->getDevice(),
            m_image,
            m_format,
            m_viewType,
            m_mipmapLevel,
            1,
            VK_IMAGE_ASPECT_COLOR_BIT,
            &m_imageInfo.imageView
        )
    );

    CHECKRESULT(
        BufferManager::bufferCreateTextureSampler(
            getRendererPointer()->getDevice(),
            m_mipmapLevel,
            VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            &m_imageInfo.sampler
        )
    );

    MipmapUtils::generateMipmaps(
        getRendererPointer()->getPhysicalDevice(),
        getRendererPointer()->getCommandPool(),
        getRendererPointer()->getGraphicsQueue(),
        m_image,
        m_extent.width,
        m_extent.height,
        m_format,
        m_mipmapLevel
    );
}

NormalTexture::NormalTexture(const std::string name)
    :TextureBase(name)
{}


CubeMapTexture::CubeMapTexture(
    const std::string name,
    const std::string& basedir,
    const VkFormat& format,
    VkImageCreateFlags flags,
    VkImageViewType viewType
) : TextureBase(name, format, flags, viewType)
{

    CHECKRESULT(
        BufferManager::bufferCreateTextureCubeMap(
            getRendererPointer()->getDevice(),
            getRendererPointer()->getVmaAllocator(),
            getRendererPointer()->getGraphicsQueue(),
            getRendererPointer()->getCommandPool(),
            basedir,
            name,
            m_format,
            flags,
            &m_image,
            &m_deviceAllocation,
            &m_extent)
    );

    CHECKRESULT(
        BufferManager::bufferCreateImageView(
            getRendererPointer()->getDevice(),
            m_image,
            m_format,
            viewType,
            1,
            6,
            VK_IMAGE_ASPECT_COLOR_BIT,
            &m_imageInfo.imageView
        )
    );

    CHECKRESULT(
        BufferManager::bufferCreateTextureSampler(
            getRendererPointer()->getDevice(),
            1 ,
            VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            &m_imageInfo.sampler)
    );
}