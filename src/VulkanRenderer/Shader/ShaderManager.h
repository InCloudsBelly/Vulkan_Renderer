#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace ShaderManager
{
    std::vector<char> getBinaryDataFromFile(const std::string& filename);
    VkShaderModule createShaderModule(
        const std::vector<char>& code
    );
    void destroyShaderModule(
        VkShaderModule& shaderModule
    );
};