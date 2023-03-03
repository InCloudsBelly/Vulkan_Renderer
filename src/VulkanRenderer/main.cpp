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
       /*     app.addSkybox("Town", "SmallTown");*/

            //app.addObjectPBR(
            //      "Gun",
            //      "gun.gltf",
            //      glm::fvec3(0.0f),
            //      glm::fvec3(0.0f),
            //      glm::fvec3(0.036f)
            //);
            /*app.addObjectPBR(
                "Helmet", "DamagedHelmet.gltf",
                glm::fvec3(0.0f),
                glm::fvec3(1.47f, 0.0f, 0.4f),
                glm::fvec3(1.0f)
            );

            app.addDirectionalLight(
                "Sun",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3(1.0f),
                glm::fvec3(0.0f),
                glm::fvec3(0.125f)
            );

            app.addPointLight(
                "Light1",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3(2.18f, 1.13f, 1.65f),
                glm::fvec3(0.05f),
                0.5f,
                3.0f
            );*/
            app.addSkybox("Apartment.hdr", "Apartment");

            app.addObjectPBR(
                "Sponza",
                "Sponza.gltf",
                glm::fvec3(0.0f),
                glm::fvec3(1.0f, -1.555, 1.0f),
                glm::fvec3(0.05f)
            );

            app.addDirectionalLight(
                "Sun",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3( -10.9f, 87.0f, -10.6f),
                glm::fvec3(30.6f, -61.0f, 0.66f),
                glm::fvec3(0.125f)
            );

            app.addPointLight(
                "PointLight1",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3(0.0f, 0.1, 2.881f),
                glm::fvec3(0.125f),
                0.5f,
                3.0f
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