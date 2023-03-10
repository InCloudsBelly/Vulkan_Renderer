#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/texture.hpp>
#include <gli/load_ktx.hpp>

#include "VulkanRenderer/Command/CommandPool.h"
#include "VulkanRenderer/Descriptor/Types/Sampler/Sampler.h"
#include "VulkanRenderer/Image/Image.h"


enum TextureType
{
    NORMAL_TEXTURE = 0,
    CUBEMAP = 1
};

enum UsageType
{
    // It's just used to color meshes.
    // (this is the most common one).
    TO_COLOR = 0,
    ENVIRONMENTAL_MAP = 1,
    IRRADIANCE_MAP = 2,
};

struct TextureToLoadInfo
{
    std::string name;
    std::string folderName;
    VkFormat    format;
    int         desiredChannels;
};

class Texture
{

public:

    Texture();
    Texture(
        const VkDevice& logicalDevice,
        const TextureType& type,
        const VkSampleCountFlagBits& samplesCount,
        const int& desiredChannels = 4,
        const UsageType& usage = UsageType::TO_COLOR
    );
    virtual ~Texture() = 0;

    const VkImageView& getImageView() const;
    const VkSampler& getSampler() const;
    const UsageType& getUsage() const;

    void destroy();

protected:

    VkDevice              m_logicalDevice;

    Image                 m_image;

    TextureType           m_type;
    UsageType             m_usage;
    int                   m_width;
    int                   m_height;
    int                   m_channels;
    int                   m_desiredChannels;
    uint32_t              m_mipLevels;
    VkSampleCountFlagBits m_samplesCount;
};