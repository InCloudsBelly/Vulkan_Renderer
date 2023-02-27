#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>

#include "VulkanRenderer/Renderer.h"

int main()
{
    Renderer  app;

    try
    {
        //app.addObject("Bunny", "stanford-bunny.obj");
        app.addSkybox("Town", "SmallTown");
        app.addObjectPBR("Helmet", "DamagedHelmet.gltf");

        app.addDirectionalLight("DirectionaLight1", "lightSphere.obj");

        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    return 0;
}