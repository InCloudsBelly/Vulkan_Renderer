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
           /* app.addSkybox("Apartment.hdr", "Apartment");


            app.addObjectPBR(
                "DamagedHelmet",
                "DamagedHelmet.gltf",
                glm::fvec3(0.0f),
                glm::fvec3(1.227f, 0.0f, 0.0f),
                glm::fvec3(1.0f)
            );

            app.addDirectionalLight(
                "Sun",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3( -10.9f, 87.0f, -10.6f),
                glm::fvec3(30.6f, -61.0f, 0.66f),
                glm::fvec3(0.125f)
            );*/
        }

        //Scene 2
        {
            app.addSkybox("Apartment.hdr", "Apartment");
            app.addObjectPBR(
                "Sponza",
                "Sponza.gltf",
                glm::fvec3(0.0f),
                glm::fvec3(1.0f, -1.555, 1.0f),
                glm::fvec3(0.05f)
            );

            app.addSpotLight(
                "Spot1",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3(0.0f),
                glm::fvec3(0.0f),
                glm::fvec3(0.0f),
                glm::fvec3(0.125f)
            );
            app.addDirectionalLight(
                "Sun",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3(1.0f, 87.0f, -49.0f),
                glm::fvec3(1.461f, 2.619f, 57.457f),
                glm::fvec3(0.125f)
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