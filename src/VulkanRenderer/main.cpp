#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>

#include "VulkanRenderer/Renderer.h"

int main()
{
    Renderer  app;

    try
    {
        // SCENE 1
        {
            app.addSkybox("Town", "SmallTown");
            app.addObjectPBR(
                "Helmet", "DamagedHelmet.gltf",
                glm::fvec3(0.0f),
                glm::fvec3(1.47f, 0.0f, 0.4f),
                glm::fvec3(1.0f)
            );
            app.addDirectionalLight(
                "DirectionaLight1",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3(2.18f, 1.13f, 1.65f),
                glm::fvec3(0.0f),
                glm::fvec3(0.05f)
            );
        }

        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    return 0;
}