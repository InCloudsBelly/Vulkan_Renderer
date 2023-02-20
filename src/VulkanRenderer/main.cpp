#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>

#include "VulkanRenderer/Renderer.h"

int main()
{
    App app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    return 0;
}