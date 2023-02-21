#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class WindowManager
{
public:
	WindowManager();
	
	void createWindow(const uint16_t width, const uint16_t height, const char* title);

	void createSurface(const VkInstance& instance);

	const VkSurfaceKHR getSurface() const;
	void getResolutionInPixels(int& width, int& height) const;

	void destroyWindow();
	void destroySurface(const VkInstance& instance);

	bool isWindowClosed() const;
	bool isAllowedToModifyTheResolution(const VkSurfaceCapabilitiesKHR& capabilities) const;

	void pollEvents();

	~WindowManager();

private:

	GLFWwindow* m_window;
	VkSurfaceKHR m_surface;
};