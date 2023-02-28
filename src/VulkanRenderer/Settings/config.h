#pragma once 

#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#include "VulkanRenderer/Descriptors/DescriptorInfo.h"

namespace Config
{
	inline const uint16_t RESOLUTION_W = 1920;
	inline const uint16_t RESOLUTION_H = 1080;

	inline const char* TITLE = "Hello Vulkan";

	// Graphic's settings
	inline const int MAX_FRAMES_IN_FLIGHT = 2;

	//Camera settings
	inline const float FOV = 45.0f;
	inline const float Z_NEAR = 0.01f;
	inline const float Z_FAR = 100.0f;


	// Textures - Naming Convention For Cubemaps
	inline const std::vector<std::string> TEXTURE_CUBEMAP_NAMING_CONV =
	{"pz","nz","py","ny","nx","px"};

	inline const std::string TEXTURE_CUBEMAP_FORMAT = "png";
}