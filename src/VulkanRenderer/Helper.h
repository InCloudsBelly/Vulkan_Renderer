#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/transform.hpp>

#define _USE_MATH_DEFINES

#include <math.h>

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include <stb_image.h>
#include <stb_image_write.h>


#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>


#define CHECKRESULT(x) { \
        VkResult retval = (x); \
        assert (retval == VK_SUCCESS); \
    }

///need only for start up
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities; ///<Surface capabilities
	std::vector<VkSurfaceFormatKHR> formats; ///<Surface formats available
	std::vector<VkPresentModeKHR> presentModes; ///<Possible present modes
};

