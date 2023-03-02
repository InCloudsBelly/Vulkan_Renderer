#include "VulkanRenderer/Camera/Types/Arcball.h"

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>

#include "VulkanRenderer/Camera/Camera.h"
#include "VulkanRenderer/Settings/config.h"


Arcball::Arcball(
    GLFWwindow*         window,
    const glm::fvec4&   pos,
    const glm::fvec4&   target,
    const float         FOV,
    const float         ratio,
    const float         zNear,
    const float         zFar
) : Camera(window,pos, target,CameraType::ARCBALL,FOV,ratio,zNear,zFar) {
    m_lastCursorPos = glm::fvec2(0.0f);
    m_currentCursorPos = glm::fvec2(0.0f);
    m_targetPos = glm::fvec4(0.0f);
}

Arcball::~Arcball() {}

glm::fvec3 Arcball::getArcballVector(const float x, const float y)
{
    glm::fvec3 P = glm::fvec3(
        1.0f * x / Config::RESOLUTION_W * 2.0f - 1.0f,
        -1.0f * (1.0f * y / Config::RESOLUTION_H * 2.0f - 1.0f),
        0.0f
    );

    float OPsquared = P.x * P.x + P.y * P.y;

    if (OPsquared <= 1.0)
    {
        P.z = glm::sqrt(1 - OPsquared);
    }
    else
    {
        P = glm::normalize(P);
    }

    return P;
}

/*
 * Algorithm taken from:
 * https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball
 */
void Arcball::updateCameraPos(glm::mat4& newRot) 
{
    double x, y;
    glfwGetCursorPos(m_opWindow, &x, &y);

    m_currentCursorPos = glm::fvec2(x, y);

    if (x != m_lastCursorPos.x || y != m_lastCursorPos.y)
    {
        glm::vec3 v1 = getArcballVector(m_lastCursorPos.x, m_lastCursorPos.y);
        glm::vec3 v2 = getArcballVector(x, y);

        float angle = glm::acos(glm::min(1.0f, glm::dot(v1, v2))) * 0.02;

        glm::vec3 axisInCameraCoord = glm::cross(v1, v2);
        glm::mat3 camera2object = glm::inverse(glm::mat3(m_view) * glm::mat3(newRot));
        glm::vec3 axisInObjectCoord = camera2object * axisInCameraCoord;

        newRot = glm::rotate(newRot, glm::degrees(angle), axisInObjectCoord);

        m_lastCursorPos = m_currentCursorPos;

        m_pos = newRot * m_pos;
    }
}

void Arcball::saveCursorPos()
{
    double x, y;
    glfwGetCursorPos(m_opWindow, &x, &y);
    m_currentCursorPos = m_lastCursorPos = glm::fvec2(x, y);
}