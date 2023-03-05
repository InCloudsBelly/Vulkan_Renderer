#pragma once

#include <vector>

#include "VulkanRenderer/Descriptor/DescriptorInfo.h"


namespace GRAPHICS_PIPELINE
{
    /////////////////////////////For PBR Models/////////////////////////////////

    namespace PBR
    {
        inline const std::vector<DescriptorInfo> UBOS_INFO = {
           {
                0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT |VK_SHADER_STAGE_FRAGMENT_BIT)
           },
           {
                1,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                (VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)
           }
        };
        inline const std::vector<DescriptorInfo> SAMPLERS_INFO = {
            {2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {3,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {4,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {5,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {6,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            // Env. Map (IMPORTANT: Always leave it positioned before the irradiance map)
            {7,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            // Irradiance Map (IMPORTANT: Always leave it positioned before the BRDF lut map)
            {8,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            // BRDF lut (IMPORTANT: Always leave it positioned before the shadow map)            
            {9,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            // Shadow Map (IMPORTANT: Always leave it as the last sampler)
            {10,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };

        // We don't count the shadow , env., BRDF and irradiance map.
        inline const uint32_t TEXTURES_PER_MESH_COUNT = SAMPLERS_INFO.size() - 4;

        inline const uint32_t SAMPLERS_PER_MESH_COUNT = SAMPLERS_INFO.size();

        inline const uint32_t UBOS_PER_MESH_COUNT = UBOS_INFO.size();
    };

    ///////////////////////////////For Skyboxes/////////////////////////////////
    namespace SKYBOX
    {
        inline const std::vector<DescriptorInfo> UBOS_INFO = {
           {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT)}
        };

        inline const std::vector<DescriptorInfo> SAMPLERS_INFO = {
           {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };

        inline const uint32_t TEXTURES_PER_MESH_COUNT = SAMPLERS_INFO.size();
        inline const uint32_t UBOS_PER_MESH_COUNT = UBOS_INFO.size();

    };

    /////////////////////////////For Light Models///////////////////////////////

    namespace LIGHT
    {
        inline const std::vector<DescriptorInfo> UBOS_INFO = {
           {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)}
        };

        inline const std::vector<DescriptorInfo> SAMPLERS_INFO = {
            {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };
        
        inline const uint32_t TEXTURES_PER_MESH_COUNT = SAMPLERS_INFO.size();
        inline const uint32_t UBOS_PER_MESH_COUNT = UBOS_INFO.size();
    };


    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////FEATURES/////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////ShadowMap////////////////////////////////////

    namespace SHADOWMAP
    {
        inline const std::vector<DescriptorInfo> UBOS_INFO = {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT) }
        };

        inline const uint32_t UBOS_COUNT = UBOS_INFO.size();
        inline const uint32_t SAMPLERS_COUNT = 0;
    };
};


    