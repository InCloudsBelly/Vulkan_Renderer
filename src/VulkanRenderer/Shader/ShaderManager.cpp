#include "VulkanRenderer/Shader/ShaderManager.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include <vulkan/vulkan.h>
#include "VulkanRenderer/Renderer.h"

std::vector<char> ShaderManager::getBinaryDataFromFile(const std::string& filename)
{
    std::ifstream file(
        std::string(SHADERS_BINARY_DIR) + "/" + filename + ".spv",
        std::ios::ate | std::ios::binary
    );

    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filename + ".spv");

    uint32_t fileSize = (uint32_t)file.tellg();
    std::vector<char> buffer(fileSize);

    // Seeks back to the beginning of the file.
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

VkShaderModule ShaderManager::createShaderModule(
    const std::vector<char>& code
) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    auto status = vkCreateShaderModule(
        getRendererPointer()->getDevice(),
        &createInfo,
        nullptr,
        &shaderModule
    );

    if (status != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module!");

    return shaderModule;
}

void ShaderManager::destroyShaderModule(
    VkShaderModule& shaderModule
) {
    vkDestroyShaderModule(getRendererPointer()->getDevice(), shaderModule, nullptr);
}