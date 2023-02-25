#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Make it Singleton!
class Window
{
public:
	Window();
   ~Window();
	
	void createWindow(const uint16_t width, const uint16_t height, const char* title);

	void createSurface(const VkInstance& instance);

	const VkSurfaceKHR getSurface() const;
	void getResolutionInPixels(int& width, int& height) const;

	void destroyWindow();
	void destroySurface(const VkInstance& instance);

	bool isWindowClosed() const;
	bool isAllowedToModifyTheResolution(const VkSurfaceCapabilitiesKHR& capabilities) const;

	void pollEvents();

	const GLFWwindow* get() const;

private:

	// ImGui needs to access to non-const GLFWwindow.
	friend class GUI;
	GLFWwindow* m_window;
	VkSurfaceKHR m_surface;
};