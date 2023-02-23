#pragma once 
#include <vulkan/vulkan.h>

namespace Config
{
	inline const uint16_t RESOLUTION_W = 1920;
	inline const uint16_t RESOLUTION_H = 1080;

	inline const char* TITLE = "Hello Vulkan";

	// Graphic's settings
	inline const int MAX_FRAMES_IN_FLIGHT = 2;
}