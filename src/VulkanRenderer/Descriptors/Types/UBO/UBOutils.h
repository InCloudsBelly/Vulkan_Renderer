#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanRenderer/Descriptors/Types/UBO/UBO.h"

namespace UBOutils
{
    glm::mat4 getUpdatedModelMatrix(
        const glm::fvec4    actualPos,
        const glm::fvec3    actualRot,
        const glm::fvec3    actualSize
    );
    glm::mat4 getUpdatedViewMatrix(
        const glm::fvec3&   cameraPos,
        const glm::fvec3&   centerPos,
        const glm::fvec3&   upAxis
    );
    glm::mat4 getUpdatedProjMatrix(
        const float         vfov,
        const float         aspect,
        const float         nearZ,
        const float         farZ
    );

    void updateUBO(const VkDevice& logicalDevice, UBO & ubo, const size_t size, void * dataToSend, const uint32_t& currentFrame);
};