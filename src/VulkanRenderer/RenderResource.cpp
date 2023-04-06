

#include <thread>
#include <vector>

#include "RenderResource.h"
#include "VulkanRenderer/Renderer.h"

void RenderResource::loadModels(const std::vector<ModelInfo>& modelsToLoadInfo)
{
    std::vector<std::thread> threads;

    const uint32_t maxThreadsCount = std::thread::hardware_concurrency() - 1;
    uint32_t chunckSize = ((modelsToLoadInfo.size() < maxThreadsCount) ? 1 : modelsToLoadInfo.size() / maxThreadsCount);
    const uint32_t threadsCount = ((modelsToLoadInfo.size() < maxThreadsCount) ? modelsToLoadInfo.size() : maxThreadsCount);

    for (uint32_t i = 0; i < threadsCount; i++)
    {
        if (i == threadsCount - 1 && maxThreadsCount < modelsToLoadInfo.size())
        {
            chunckSize = (modelsToLoadInfo.size() - (threadsCount * chunckSize));
        }
        threads.push_back(std::thread(&RenderResource::loadModel, this, i, chunckSize, modelsToLoadInfo));
    }

    for (auto& thread : threads)
        thread.join();

    /*  if (getRenderResource()->m_objectModelIndices.size() == 0)
          throw std::runtime_error("Add at least 1 model." );
      if (getRenderResource()->m_directionalLightIndex == -1)
          throw std::runtime_error("Add at least 1 directional light.");
      if (getRenderResource()->m_skyboxIndex == -1)
          throw std::runtime_error("Add at least 1 skybox.");*/
}

void RenderResource::loadModel(const uint32_t startI, const uint32_t chunckSize, const std::vector<ModelInfo>& modelsToLoadInfo)
{
    const uint32_t endI = startI + chunckSize;

    for (uint32_t i = startI; i < endI; i++)
    {
        const ModelInfo& modelInfo = modelsToLoadInfo[i];

        std::shared_ptr<Model> model = std::make_shared<Model>(modelInfo.name, modelInfo.fileName, modelInfo.folderName, modelInfo.type, modelInfo.pos, modelInfo.rot, modelInfo.size);

        if (modelInfo.type == ModelType::NORMAL_PBR)
           m_normalModels.push_back(model);
        else if (modelInfo.type == ModelType::SKYBOX)
           m_skybox = model;
        else if (modelInfo.type == ModelType::LIGHT)
        {
           m_lightModels.push_back(model);

            LightInfo lightInfo{ modelInfo.name,modelInfo.pos, modelInfo.rot, modelInfo.size, modelInfo.endPos, modelInfo.color, 1.0f, modelInfo.lType, model };
            lightInfo.m_intensity = lightInfo.m_lightType == LightType::DIRECTIONAL_LIGHT ? 15.0f : 70.0f;
           m_lightsInfo.push_back(lightInfo);
            if (lightInfo.m_lightType == LightType::DIRECTIONAL_LIGHT)
               m_directionalLightIndex =m_lightsInfo.size() - 1;
        }
    }
}

void RenderResource::uploadModels(const VkQueue& graphicsQueue, const VkCommandPool& commandPool)
{
    for (auto ptr : m_normalModels)
        ptr->upload(commandPool, graphicsQueue);

    for (auto ptr : m_lightModels)
        ptr->upload(commandPool, graphicsQueue);

    auto skybox = m_skybox;
    skybox->upload(commandPool, graphicsQueue);

}


void RenderResource::updateIBLResource(std::shared_ptr<TextureBase> brdfLUT, std::shared_ptr<TextureBase> Irradiance, std::shared_ptr<TextureBase> Env)
{
    m_IBLResource.brdfLUT = brdfLUT;
    m_IBLResource.irradiance = Irradiance;
    m_IBLResource.prefiltered_Env = Env;
}


void RenderResource::destroy()
{
	for (auto& meshInfo : m_meshInfoMap)
	{
		//destroy meshData
		vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), *meshInfo.second.ref_mesh->indexBuffer, meshInfo.second.ref_mesh->indexAllocation);
		vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), *meshInfo.second.ref_mesh->vertexBuffer, meshInfo.second.ref_mesh->vertexAllocation);

		//destroy materialData
		if (meshInfo.second.ref_material != nullptr)
		{
			meshInfo.second.ref_material->colorTexture->destroy();
			meshInfo.second.ref_material->metallic_RoughnessTexture->destroy();
			meshInfo.second.ref_material->emissiveTexture->destroy();
			meshInfo.second.ref_material->AOTexture->destroy();
			meshInfo.second.ref_material->normalTexture->destroy();
		}
	}


	if (m_skyboxCubeMap != nullptr)
		m_skyboxCubeMap->destroy();
	if (m_defaultTexture != nullptr)
		m_defaultTexture->destroy();
}
