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
        app.addLightModel("Light1","lightSphere.obj",glm::fvec4(0.0f, 0.0f, 1.0f, 1.0f));
        app.addLightModel("Light2","lightSphere.obj",glm::fvec4(1.0f, 0.0f, 0.0f, 1.0f));

        app.addNormalModel("Bunny", "stanford-bunny.obj");

        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    return 0;
}