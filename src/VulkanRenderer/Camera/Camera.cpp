
#include "Camera.h"
#include <iostream>
#include <functional>

#include <GLFW/glfw3.h>

#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Renderer.h"

Camera::Camera(glm::fvec3 vCameraPos, glm::fvec3 vCameraFront, glm::fvec3 vUpVector) :m_CameraPos(vCameraPos), m_CameraFront(vCameraFront), m_UpVector(vUpVector), m_PrevCameraPos(vCameraPos), m_PrevCameraFront(vCameraFront)
{
	m_CameraRight = glm::normalize(glm::cross(m_UpVector, -m_CameraFront));  //Fixed Me: - ?
	m_Pitch = asin(m_CameraFront.y);
	m_Yaw = asin(m_CameraFront.z / cos(m_Pitch));

	init();
}

Camera::~Camera()
{
}

//************************************************************************************
//Function:
void Camera::init()
{
	InputManager* pInputManager = getInputManager();
	pInputManager->registerKeyCallbackFunc(std::bind(&Camera::processMovement4KeyCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	pInputManager->registerCursorCallbackFunc(std::bind(&Camera::processRotate4CursorCallback, this, std::placeholders::_1, std::placeholders::_2));
	pInputManager->registerScrollCallbackFunc(std::bind(&Camera::processScroll4ScrollCallback, this, std::placeholders::_1, std::placeholders::_2));
	pInputManager->registerMouseButtonCallbackFunc(std::bind(&Camera::processMouseButton4MouseButtonCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

//************************************************************************************
//Function:
void Camera::processMovement4KeyCallback(int vKey, int vScancode, int vAction, int vMode)
{
	if (vKey == GLFW_KEY_Z && vAction == GLFW_PRESS)
		m_MoveSpeed *= 1.2f;
	if (vKey == GLFW_KEY_C && vAction == GLFW_PRESS)
		m_MoveSpeed /= 1.2f;
}

//************************************************************************************
//Function:
void Camera::processRotate4CursorCallback(double vPosX, double vPosY)
{
	if (m_IsEnableCursor)
	{
		std::array<double, 2> CursorOffset = getInputManager()->getCursorOffset();
		CursorOffset[0] *= m_Sensitivity;
		CursorOffset[1] *= m_Sensitivity;
		m_Yaw += glm::radians(CursorOffset[0]);
		m_Pitch +=  glm::radians(CursorOffset[1]);
		if (m_Pitch > glm::radians(89.0))
			m_Pitch = glm::radians(89.0);
		else if (m_Pitch < glm::radians(-89.0))
			m_Pitch = glm::radians(-89.0);
		m_CameraFront.x = cos(m_Pitch) * cos(m_Yaw);
		m_CameraFront.y = sin(m_Pitch);
		m_CameraFront.z = cos(m_Pitch) * sin(m_Yaw);
		m_CameraFront = glm::normalize(m_CameraFront);
		m_CameraRight = normalize(cross(m_UpVector, -m_CameraFront));
	}
}

//************************************************************************************
//Function:
void Camera::processScroll4ScrollCallback(double vOffsetX, double vOffsetY)
{
	if (m_Fov >= 1.0 && m_Fov <= 45.0)
		m_Fov -= vOffsetY;
	if (m_Fov < 1.0)
		m_Fov = 1.0;
	else if (m_Fov > 45.0)
		m_Fov = 45.0;
}

//************************************************************************************
//Function:
void Camera::processMouseButton4MouseButtonCallback(int vButton, int vAction, int vMods)
{
	if (vButton == GLFW_MOUSE_BUTTON_RIGHT && vAction == GLFW_PRESS)
	{
		m_IsEnableCursor = !m_IsEnableCursor;
	}
}

//************************************************************************************
//Function:
double Camera::getCameraFov() const
{
	return m_Fov;
}

//************************************************************************************
//Function:
double Camera::getCameraNear() const
{
	return m_Near;
}

//************************************************************************************
//Function:
double Camera::getCameraFar() const
{
	return m_Far;
}

//************************************************************************************
//Function:
const glm::fvec3& Camera::getCameraPos() const
{
	return m_CameraPos;
}

//************************************************************************************
//Function:
glm::fvec3 Camera::getLookAtPos() const
{
	return m_CameraPos + m_CameraFront;
}

//************************************************************************************
//Function:
glm::fvec3 Camera::getCameraFront() const
{
	return m_CameraFront;
}

//************************************************************************************
//Function:
const glm::fvec3& Camera::getCameraUp() const
{
	return m_UpVector;
}

//************************************************************************************
//Function:
glm::mat4 Camera::getViewMatrix() const
{
	return glm::lookAt(m_CameraPos, m_CameraPos + m_CameraFront, m_UpVector);
}

//************************************************************************************
//Function:
glm::mat4 Camera::getProjectionMatrix() const
{
	//return glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 1000.0f);;
	glm::mat4 proj = glm::perspective(glm::radians(m_Fov), (double)Config::RESOLUTION_W / Config::RESOLUTION_H, m_Near, m_Far);

	proj[1][1] *= -1;
	return proj;
}

//************************************************************************************
//Function:
void Camera::update(float deltatime)
{
	m_PrevCameraPos = m_CameraPos;
	m_PrevCameraFront = m_CameraFront;

	InputManager* pInputManager = getInputManager();
	float MoveDistance = deltatime * m_MoveSpeed;
	if (pInputManager->getKeyStatus()[GLFW_KEY_W])
		m_CameraPos += MoveDistance * m_CameraFront;
	if (pInputManager->getKeyStatus()[GLFW_KEY_S])
		m_CameraPos -= MoveDistance * m_CameraFront;
	if (pInputManager->getKeyStatus()[GLFW_KEY_D])
		m_CameraPos += MoveDistance * m_CameraRight;
	if (pInputManager->getKeyStatus()[GLFW_KEY_A])
		m_CameraPos -= MoveDistance * m_CameraRight;
	if (pInputManager->getKeyStatus()[GLFW_KEY_Q])
		m_CameraPos -= MoveDistance * 0.8f * glm::cross(m_CameraRight, m_CameraFront);
	if (pInputManager->getKeyStatus()[GLFW_KEY_E])
		m_CameraPos += MoveDistance * 0.8f * glm::cross(m_CameraRight, m_CameraFront);
	
}

//************************************************************************************
//Function:
glm::mat4 Camera::getPrevViewMatrix() const
{
	return glm::lookAt(m_PrevCameraPos, m_PrevCameraPos + m_PrevCameraFront, m_UpVector);
}