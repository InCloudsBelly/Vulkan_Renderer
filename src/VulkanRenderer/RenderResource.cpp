
#include "RenderResource.h"

void RenderResource::updateIBLResource(std::shared_ptr<TextureBase> brdfLUT, std::shared_ptr<TextureBase> Irradiance, std::shared_ptr<TextureBase> Env)
{
	m_IBLResource.brdfLUT = brdfLUT;
	m_IBLResource.irradiance = Irradiance;
	m_IBLResource.prefiltered_Env = Env;
}
