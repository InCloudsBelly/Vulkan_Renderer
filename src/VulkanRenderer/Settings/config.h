#pragma once 

#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#include "VulkanRenderer/Descriptor/DescriptorInfo.h"

namespace Config
{
	inline const uint16_t RESOLUTION_W = 1920;
	inline const uint16_t RESOLUTION_H = 1080;

	inline const char* WINDOW_TITLE = "Hello Vulkan";

	// Graphic's settings
	inline const int MAX_FRAMES_IN_FLIGHT = 2;

	//Camera settings
	inline const float FOV = 60.0f;
	inline const float Z_NEAR = 0.1f;
	inline const float Z_FAR = 100.0f;


	//LIGHT Camera settings
	inline const float Z_NEAR_SHADOW = 1.0f;
	inline const float Z_FAR_SHADOW = 100.0f;

	// Scene
	inline const uint32_t LIGHTS_COUNT = 10;

	// BRDF
	inline const uint32_t BRDF_WIDTH = 256;
	inline const uint32_t BRDF_HEIGHT = 256;

	// Prefiltered Irradiance;
	inline const VkFormat PREF_IRRADIANCE_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
	inline const uint32_t PREF_IRRADIANCE_DIM = 64;

	// Prefiltered Env. Map
	inline const VkFormat PREF_ENV_MAP_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
	inline const uint32_t PREF_ENV_MAP_DIM = 512;
}