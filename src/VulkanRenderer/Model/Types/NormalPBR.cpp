#include "VulkanRenderer/Model/Types/NormalPBR.h"

#include <iostream>

#include "VulkanRenderer/Textures/Texture.h"
#include "VulkanRenderer/Settings/Config.h"
#include "VulkanRenderer/Descriptors/Types/DescriptorTypes.h"
#include "VulkanRenderer/Descriptors/Types/UBO/UBOutils.h"
#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Buffers/BufferManager.h"
#include "VulkanRenderer/Model/Types/Light.h"


NormalPBR::NormalPBR(const std::string& name, const std::string& modelFileName,
	const glm::fvec4& pos,
	const glm::fvec3& rot,
	const glm::fvec3& size
) : Model(name, ModelType::NORMAL_PBR, pos, rot, size) 
{
	loadModel((std::string(MODEL_DIR) + modelFileName).c_str());
}

NormalPBR::~NormalPBR() {}

void NormalPBR::destroy(const VkDevice& logicalDevice)
{
	m_ubo.destroyUniformBuffersAndMemories(logicalDevice);
	m_uboLights.destroyUniformBuffersAndMemories(logicalDevice);

	for (auto& texture : m_texturesLoaded)
		texture->destroyTexture(logicalDevice);

	for (auto& mesh : m_meshes)
	{
		BufferManager::destroyBuffer(logicalDevice, mesh.m_vertexBuffer);
		BufferManager::destroyBuffer(logicalDevice, mesh.m_indexBuffer);

		BufferManager::freeMemory(logicalDevice, mesh.m_vertexMemory);
		BufferManager::freeMemory(logicalDevice, mesh.m_indexMemory);
	}
}

std::string NormalPBR::getMaterialTextureName(aiMaterial* material,const aiTextureType& type,const std::string& typeName, const std::string& defaultTextureFile)
{
	if (material->GetTextureCount(type) > 0)
	{
		// TODO: do something when there are many textures of the same type.

		//for (size_t i = 0; i < material->GetTextureCount(type); i++)
		//{
		//   aiString str;
		//   material->GetTexture(type, i, &str);
		//}
		aiString str;
		material->GetTexture(type, 0, &str);

		return str.C_Str();

	}
	else {
		if (typeName == "NORMALS")
			std::cout << " NSADSADSADSA NO HAY NORMALS\n";
		return defaultTextureFile;
	}
}

void NormalPBR::processMesh(aiMesh* mesh, const aiScene* scene)
{
	Mesh<Attributes::PBR::Vertex> newMesh;

	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		Attributes::PBR::Vertex vertex{};

		//Position
		vertex.pos = {mesh->mVertices[i].x,mesh->mVertices[i].y,mesh->mVertices[i].z};
		if (mesh->mNormals != NULL)
		{
			vertex.normal = glm::normalize(glm::fvec3(mesh->mNormals[i].x,mesh->mNormals[i].y,mesh->mNormals[i].z));
		}
		else {
			std::cout << " NO HAY NORMALS !\n";
			vertex.normal = glm::fvec3(1.0f);
		}

		//TexCoords
		if (mesh->mTextureCoords[0] != NULL)
		{
			vertex.texCoord = {mesh->mTextureCoords[0][i].x,mesh->mTextureCoords[0][i].y,};
			newMesh.m_hasTextureCoords = true;
		}
		else
		{
			vertex.texCoord = glm::fvec3(1.0f);
			newMesh.m_hasTextureCoords = false;
		}

		//Tangents
		if (mesh->mTangents != NULL)
		{
			vertex.tangent = glm::normalize(glm::fvec3(mesh->mTangents[i].x,mesh->mTangents[i].y,mesh->mTangents[i].z));
		}
		else
			vertex.tangent = glm::fvec3(1.0f);

		//glm::vec3 bitangent = glm::cross(vertex.normal, tangent);

		vertex.posInLightSpace = glm::fvec4(1.0f);

		newMesh.m_vertices.push_back(vertex);
	}

	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		auto face = mesh->mFaces[i];
		for (size_t j = 0; j < face.mNumIndices; j++)
			newMesh.m_indices.push_back(face.mIndices[j]);
	}


	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		TextureToLoadInfo info;
		info.name = getMaterialTextureName(material, aiTextureType_DIFFUSE, "DIFFUSE", "textures/default.jpg");
		info.format = VK_FORMAT_R8G8B8A8_SRGB;
		newMesh.m_texturesToLoadInfo.push_back(info);

		info.name = getMaterialTextureName(material,aiTextureType_UNKNOWN,"METALIC_ROUGHNESS", "textures/default.jpg");
		info.format = VK_FORMAT_R8G8B8A8_SRGB;
		newMesh.m_texturesToLoadInfo.push_back(info);

		info.name = getMaterialTextureName(material,aiTextureType_NORMALS,"NORMALS", "textures/default.jpg");
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		newMesh.m_texturesToLoadInfo.push_back(info);
	}
	m_meshes.push_back(newMesh);
}

void NormalPBR::createUniformBuffers(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice,const uint32_t& uboCount) 
{
	m_ubo.createUniformBuffers(physicalDevice, logicalDevice, uboCount, sizeof(DescriptorTypes::UniformBufferObject::NormalPBR));
	m_uboLights.createUniformBuffers(physicalDevice, logicalDevice, uboCount, sizeof(DescriptorTypes::UniformBufferObject::LightInfo) * 10);
}

void NormalPBR::createDescriptorSets(const VkDevice& logicalDevice,const VkDescriptorSetLayout& descriptorSetLayout, const ShadowMap* shadowMap, DescriptorPool& descriptorPool)
{
	std::vector<UBO*> opUBOs = { &m_ubo, &m_uboLights };

	for (auto& mesh : m_meshes)
	{
		mesh.m_descriptorSets = DescriptorSets(
			logicalDevice,
			GRAPHICS_PIPELINE::PBR::UBOS_INFO,
			GRAPHICS_PIPELINE::PBR::SAMPLERS_INFO,
			mesh.m_textures,
			&shadowMap->getShadowMapView(),
			&shadowMap->getSampler(),
			opUBOs,
			descriptorSetLayout,
			descriptorPool
		);
	}
}

void NormalPBR::uploadVertexData(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice,VkQueue& graphicsQueue,CommandPool& commandPool) 
{

	for (auto& mesh : m_meshes)
	{
		// Vertex Buffer(with staging buffer)
		BufferManager::createBufferAndTransferToDevice(
			commandPool,
			physicalDevice,
			logicalDevice,
			mesh.m_vertices.data(),
			sizeof(mesh.m_vertices[0]) * mesh.m_vertices.size(),
			graphicsQueue,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			mesh.m_vertexMemory,
			mesh.m_vertexBuffer
		);

		// Index Buffer(with staging buffer)
		BufferManager::createBufferAndTransferToDevice(
			commandPool,
			physicalDevice,
			logicalDevice,
			mesh.m_indices.data(),
			sizeof(mesh.m_indices[0]) * mesh.m_indices.size(),
			graphicsQueue,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			mesh.m_indexMemory,
			mesh.m_indexBuffer
		);
	}
}

/*
 * Creates and loads all the samplers used in the shader of each mesh.
 */
void NormalPBR::createTextures(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice, const VkSampleCountFlagBits& samplesCount, CommandPool& commandPool, VkQueue& graphicsQueue)
{
	const size_t nTextures = GRAPHICS_PIPELINE::PBR::TEXTURES_PER_MESH_COUNT;

	for (auto& mesh : m_meshes)
	{
		// Samplers of textures.
		for (size_t i = 0; i < nTextures; i++)
		{
			auto it = (m_texturesID.find(mesh.m_texturesToLoadInfo[i].name));

			if (it == m_texturesID.end())
			{
				mesh.m_textures.push_back(std::make_shared<Texture>(physicalDevice,logicalDevice,mesh.m_texturesToLoadInfo[i],false,samplesCount,commandPool,graphicsQueue));
				m_texturesLoaded.push_back(mesh.m_textures[i]);
				m_texturesID[mesh.m_texturesToLoadInfo[i].name] = (m_texturesLoaded.size() - 1);
			}
			else
				mesh.m_textures.push_back(m_texturesLoaded[it->second]);
		}
	}
}

const glm::mat4& NormalPBR::getModelM() const
{
	return m_dataInShader.model;
}

void NormalPBR::updateUBO(
	const VkDevice&				logicalDevice,
	const glm::vec4&			cameraPos,
	const glm::mat4&			view,
	const glm::mat4&			proj,
	const glm::mat4&			lightSpace,
	const int&					lightsCount,
	const std::vector<std::shared_ptr<Model>>& models,
	const uint32_t&				currentFrame
) {

	m_dataInShader.model = UBOutils::getUpdatedModelMatrix(m_pos, m_rot, m_size);
	m_dataInShader.view = view;
	m_dataInShader.proj = proj;
	m_dataInShader.lightSpace = lightSpace;

	m_dataInShader.cameraPos = cameraPos;
	m_dataInShader.lightsCount = lightsCount;

	size_t size = sizeof(m_dataInShader);
	UBOutils::updateUBO(logicalDevice, m_ubo, size, &m_dataInShader, currentFrame);
}

void NormalPBR::updateUBOlightsInfo(
	const VkDevice& logicalDevice,
	const std::vector<size_t> lightModelIndices,
	const std::vector<std::shared_ptr<Model>>& models,
	const uint32_t& currentFrame
) {
	for (size_t i = 0; i < lightModelIndices.size(); i++)
	{
		size_t j = lightModelIndices[i];

		if (auto pModel = std::dynamic_pointer_cast<Light>(models[j]))
		{
			m_lightsInfo[i].pos = pModel->getPos();
			m_lightsInfo[i].color = pModel->getColor();
			m_lightsInfo[i].dir = pModel->getTargetPos() - pModel->getPos();
			m_lightsInfo[i].attenuation = pModel->getAttenuation();
			m_lightsInfo[i].radius = pModel->getRadius();
			m_lightsInfo[i].intensity = pModel->getIntensity();
			m_lightsInfo[i].type = (int)pModel->getLightType();
		}
	}
	size_t size = sizeof(m_lightsInfo[0]) * 10;

	UBOutils::updateUBO(logicalDevice,m_uboLights,size,&m_lightsInfo,currentFrame);
}
