#pragma once

#include <vector>

#include "VulkanRenderer/Descriptor/DescriptorManager.h"


namespace GRAPHICS_PIPELINE
{
    /////////////////////////////For PBR Models/////////////////////////////////

    namespace PBR
    {
        inline const std::vector<DescriptorInfo> DESCRIPTORS_INFO = {
            {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)},
            {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},

            {2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {3,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {4,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {5,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {6,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            
            // Irradiance Map (IMPORTANT: Always leave it positioned before the BRDF lut map)
            {7,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            // BRDF lut (IMPORTANT: Always leave it positioned before the pref. env. map)            
            {8,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}, 
            // Prefiltered env. map (IMPORTANT: Always leave it positioned before the shadowMap)
            {9,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            // Shadow Map (IMPORTANT: Always leave it as the last sampler)
            {10,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };
    };

    ///////////////////////////////For Skyboxes/////////////////////////////////
    namespace SKYBOX
    {
        inline const std::vector<DescriptorInfo> DESCRIPTORS_INFO = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT)},

            {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };

        inline const uint32_t TEXTURES_PER_MESH_COUNT = 1;
        inline const uint32_t UBOS_PER_MESH_COUNT = 1;

    };

    /////////////////////////////For Light Models///////////////////////////////

    namespace LIGHT
    {
        inline const std::vector<DescriptorInfo> DESCRIPTORS_INFO = {
           {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)},

           {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };
        
        inline const uint32_t TEXTURES_PER_MESH_COUNT = 1;
        inline const uint32_t UBOS_PER_MESH_COUNT = 1;
    };


    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////FEATURES/////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////ShadowMap////////////////////////////////////
    namespace SHADOWMAP
    {
        inline const std::vector<DescriptorInfo> DESCRIPTORS_INFO = {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT) }
        };

        inline const uint32_t UBOS_COUNT = 1;
        inline const uint32_t SAMPLERS_COUNT = 0;
    };

    //////////////////////////Prefiltered_Irradiance///////////////////////////////
    namespace PREFILTER_IRRADIANCE
    {
        inline const std::vector<DescriptorInfo> DESCRIPTORS_INFO = {
           {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };

        inline const uint32_t UBOS_COUNT = 0;
        inline const uint32_t SAMPLERS_COUNT = 1;
    };

    ///////////////////////////////Prefiltered_Env///////////////////////////////////
    namespace PREFILTER_ENV_MAP
    {
        inline const std::vector<DescriptorInfo> DESCRIPTORS_INFO = {
           {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };

        inline const uint32_t UBOS_COUNT = 0;
        inline const uint32_t SAMPLERS_COUNT = 1;
    };

    ///////////////////////////////Deferred Rendering///////////////////////////////////
    namespace DEFERRED_OFF
    {
        inline const std::vector<DescriptorInfo> DESCRIPTORS_INFO = {
            {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)},

            
            {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {3,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {4,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {5,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };
    };

    namespace DEFERRED_ON
    {
        inline const std::vector<DescriptorInfo> DESCRIPTORS_INFO = {
            //Normal Infos
            {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            // Lights infos
            {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},

            // Attachment 
            {2,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {3,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {4,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {5,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {6,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {7,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},

            //IBL
            {8,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {9,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {10,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            
            //Shadow
            {11,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}

        };
    }

    namespace SH_LIGHTING
    {
        inline const std::vector<DescriptorInfo> DESCRIPTORS_INFO = {
            {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)},
            {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},

            {2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {3,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {4,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {5,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},
            {6,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)},

            // SH_BRDF
            {7,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,(VkShaderStageFlagBits)(VK_SHADER_STAGE_FRAGMENT_BIT)}
        };
    };

};


