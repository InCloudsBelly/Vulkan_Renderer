#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
	Camera(glm::fvec3 vCameraPos = glm::fvec3(0.0, 0.0, 3.0), glm::fvec3 vCameraFront = glm::fvec3(0.0, 0.0, -1.0), glm::fvec3 vUpVector = glm::fvec3(0.0, 1.0, 0.0));
	~Camera();

	void init();
	void update(float delta);

	double   getCameraFov() const;
	double   getCameraNear() const;
	double   getCameraFar() const;
	glm::mat4  getViewMatrix() const;  //not need const reference
	glm::mat4  getPrevViewMatrix() const;
	glm::mat4  getProjectionMatrix() const;
	glm::fvec3 getLookAtPos() const;
	glm::fvec3 getCameraFront() const;
	const glm::fvec3& getCameraPos() const;
	const glm::fvec3& getCameraUp() const;

	void setCameraPos(glm::fvec3 vCameraPos) { m_CameraPos = vCameraPos; }
	void setCameraFront(glm::fvec3 vCameraFront) { m_CameraFront = vCameraFront; }
	void setFarPlane(double vFarPlane) { m_Far = vFarPlane; }
	void setMoveSpeed(double vMoveSpeed) { m_MoveSpeed = vMoveSpeed; }
	void setFov(double vFov) { m_Fov = vFov; }
	void setEnableCursor(bool vIsEnableCursor) { m_IsEnableCursor = vIsEnableCursor; }

private:
	void processMovement4KeyCallback(int vKey, int vScancode, int vAction, int vMode);
	void processRotate4CursorCallback(double vPosX, double vPosY);
	void processScroll4ScrollCallback(double vOffsetX, double vOffsetY);
	void processMouseButton4MouseButtonCallback(int vButton, int vAction, int vMods);

	glm::fvec3 m_CameraPos;
	glm::fvec3 m_PrevCameraPos;
	glm::fvec3 m_UpVector;		//up vector never change, so no need prevUpVector
	glm::fvec3 m_CameraFront;
	glm::fvec3 m_PrevCameraFront;
	glm::fvec3 m_CameraRight = glm::vec3(0.0);
	glm::mat4 m_ViewMatrix = glm::mat4();
	glm::mat4 m_ProjectionMatrix = glm::mat4();
	double m_Pitch = 0.0;
	double m_Yaw = 0.0;
	double m_Fov = 45.0;
	double m_MoveSpeed = 0.005;
	double m_Sensitivity = 0.2;
	double m_Near = 0.1;
	double m_Far = 100.0;
	bool m_IsEnableCursor = true;
};