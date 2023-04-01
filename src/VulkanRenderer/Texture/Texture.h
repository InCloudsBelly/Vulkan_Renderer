#pragma once

#include <string>
#include <vulkan/vulkan.h>


#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/RenderDataTypes.h"

class TextureBase
{
public:
    TextureBase() {};

    TextureBase(const std::string name) { m_name = name; }

    TextureBase(
        const std::string name,
        const VkFormat& format,
        const VkImageCreateFlags flags,
        const VkImageViewType viewType
    ) :m_name(name), m_format(format), m_imageFlag(flags), m_viewType(viewType) 
    {}

    const std::string&              getName() const                 { return m_name; }
    const VkImage&                  getImage() const                { return m_image; }
    const VkDescriptorImageInfo&    getDescriptorImageInfo() const  { return m_imageInfo; }
    const VkImageView&              getImageView() const            { return m_imageInfo.imageView; }
    const VkSampler&                getSampler() const              { return m_imageInfo.sampler; }
    const VmaAllocation&            getAllocation() const           { return m_deviceAllocation; }
    const VkExtent2D&               getExtent() const               { return m_extent; }
    const VkFormat&                 getFormat() const               { return m_format; }


    std::string&            getName()               { return m_name; }
    VkImage&                getImage()              { return m_image; }
    VkDescriptorImageInfo&  getDescriptorImageInfo(){ return m_imageInfo; }
    VkImageView&            getImageView()          { return m_imageInfo.imageView; }
    VkSampler&              getSampler()            { return m_imageInfo.sampler; }
    VmaAllocation&          getAllocation()         { return m_deviceAllocation; }
    VkExtent2D&             getExtent()             { return m_extent; }
    VkFormat&               getFormat()             { return m_format; }

    void destroy() ;

protected:

    VkImage m_image = VK_NULL_HANDLE; ///<image handle

    VkDescriptorImageInfo m_imageInfo = {
        VK_NULL_HANDLE, ///<sampler
        VK_NULL_HANDLE, ///<image view
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ///<layout
    };
    VmaAllocation m_deviceAllocation; ///<VMA allocation info
    VkExtent2D m_extent = { 0, 0 }; ///<map extent

    uint32_t            m_mipmapLevel;
    VkFormat            m_format; ///<texture format
    VkImageCreateFlags  m_imageFlag;
    VkImageViewType     m_viewType;

    std::string         m_name;
};


class NormalTexture : public TextureBase
{
public:
    NormalTexture(
        const std::string name,
        const std::string& basedir,
        const VkFormat& format,
        const VkImageCreateFlags flags = 0,
        const VkImageViewType viewtype = VK_IMAGE_VIEW_TYPE_2D
    );

    ///Empty constructor
    NormalTexture(const std::string name);

    ~NormalTexture() {}
 
private:
};


class CubeMapTexture : public TextureBase
{
public:
    CubeMapTexture(
        const std::string name,
        const std::string& basedir,
        const VkFormat& format,
        VkImageCreateFlags flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        VkImageViewType viewtype = VK_IMAGE_VIEW_TYPE_CUBE
    );

    ~CubeMapTexture() {}

    ///Empty constructor
    CubeMapTexture(const std::string name) : TextureBase(name) {}


private:
};