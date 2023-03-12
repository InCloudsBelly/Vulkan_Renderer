#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>

#include "VulkanRenderer/Renderer.h"

/* Commands:
*
*   - addSkybox(
*        fileName,
*        folderName
*     );
*   - addObjectPBR(
*        name,
*        folderName,
*        fileName,
*        position,
*        rotation,
*        size
*     );
*   - addDirectionalLight(
*        name,
*        folderName,
*        fileName,
*        color,
*        position,
*        targetPosition,
*        size
*     );
*   - addSpotLight(
*        name,
*        folderName,
*        fileName,
*        color,
*        position,
*        targetPosition,
*        rotation,
*        size
*     );
*   - addPointLight(
*        name,
*        folderName,
*        fileName,
*        color,
*        position,
*        size
*     );
*/

int main()
{
    Renderer  app;

    try
    {
        // SCENE 1
        {
            app.addSkybox("sky.hdr", "DaySky");
            app.addObjectPBR(
                "Sponza",
                "sponzaTGA",
                "SponzaPBR.obj",
                glm::fvec3(0.0f),
                glm::fvec3(1.0f, -1.555, 1.0f),
                glm::fvec3(1.0f)
            );
            //app.addPointLight(
            //      "Point",
            //      "lightSphere.obj",
            //      glm::fvec3(1.0f),
            //      glm::fvec3(0.0f),
            //      glm::fvec3(0.125f)
            //);
            //app.addSpotLight(
            //      "Spot1",
            //      "lightSphereDefault",
            //      "lightSphere.obj",
            //      glm::fvec3(1.0f),
            //      glm::fvec3(0.0f),
            //      glm::fvec3(0.0f),
            //      glm::fvec3(0.0f),
            //      glm::fvec3(0.125f)
            //);
            app.addDirectionalLight(
                "Sun",
                "lightSphereDefault",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3(1.2f, 13.3f, 2.14f),
                glm::fvec3(5.735f, -40.0f, 2.14f),
                glm::fvec3(0.3f)
            );
        }


        // SCENE 2

       /* {
            app.addSkybox("Apartment.hdr", "Apartment");
            app.addObjectPBR(
                "DamagedHelmet",
                "damagedHelmet",
                "DamagedHelmet.gltf",
                glm::fvec3(0.0f),
                glm::fvec3(0.0f),
                glm::fvec3(1.0f)
            );
            app.addDirectionalLight(
                "Sun",
                "lightSphereDefault",
                "lightSphere.obj",
                glm::fvec3(1.0f),
                glm::fvec3(1.0f, 87.0f, -49.0f),
                glm::fvec3(1.461f, 2.619f, 57.457f),
                glm::fvec3(0.125f)
            );
        }*/
 

        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    return 0;
}