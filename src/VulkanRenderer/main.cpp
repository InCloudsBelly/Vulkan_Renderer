#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>

#include "VulkanRenderer/Renderer.h"

int main()
{
    Renderer  app;

    try
    {
        /*
        * Normal Model -> Model that interacts with light.
        * Light Model  -> Model that produces light.
        */
        app.addNormalModel("Bunny", "stanford-bunny.obj");
        app.addLightModel("Light", "lightSphere.obj");

        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    return 0;
}