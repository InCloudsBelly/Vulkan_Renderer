
#include "RenderResource.h"
#include "VulkanRenderer/Renderer.h"

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
