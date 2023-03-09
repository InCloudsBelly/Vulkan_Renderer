#include "VulkanRenderer/Model/Types/NormalPBR.h"

#include <iostream>


#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Descriptor/Types/DescriptorTypes.h"
#include "VulkanRenderer/Descriptor/Types/UBO/UBOutils.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Model/Types/Light.h"
#include "VulkanRenderer/Math/MathUtils.h"
#include "VulkanRenderer/Texture/Type/NormalTexture.h"
#include "VulkanRenderer/Command/CommandManager.h"

NormalPBR::NormalPBR(const ModelInfo& modelInfo)
	: Model(modelInfo.name, ModelType::NORMAL_PBR, glm::fvec4(modelInfo.pos, 1.0f), modelInfo.rot, modelInfo.size) 
{
	loadModel((std::string(MODEL_DIR) + modelInfo.modelFileName).c_str());
}

NormalPBR::~NormalPBR() {}

void NormalPBR::destroy(const VkDevice& logicalDevice)
{
	m_ubo->destroy();
	m_uboLights->destroy();

	for (auto& texture : m_texturesLoaded)
		texture->destroy();

	for (auto& mesh : m_meshes)
	{
		BufferManager::destroyBuffer(logicalDevice, mesh.vertexBuffer);
		BufferManager::destroyBuffer(logicalDevice, mesh.indexBuffer);

		BufferManager::freeMemory(logicalDevice, mesh.vertexMemory);
		BufferManager::freeMemory(logicalDevice, mesh.indexMemory);
	}
}

std::string NormalPBR::getMaterialTextureName(aiMaterial* material,const aiTextureType& type,const std::string& typeName, const std::string& defaultTextureFile)
{
	if (material->GetTextureCount(type) > 0)
	{
		aiString str;
		material->GetTexture(type, 0, &str);

		if (typeName == "NORMALS")
			m_hasNormalMap = true;

		return str.C_Str();

	}
	else
	{
		if (typeName == "NORMALS")
			m_hasNormalMap = false;

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
		else
			throw std::runtime_error("Mesh doesn't have normals!");


		//TexCoords
		if (mesh->mTextureCoords[0] != NULL)
		{
			vertex.texCoord = {mesh->mTextureCoords[0][i].x,mesh->mTextureCoords[0][i].y,};
		}
		else
		{
			vertex.texCoord = glm::fvec3(1.0f);
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

		newMesh.vertices.push_back(vertex);
	}

	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		auto face = mesh->mFaces[i];
		for (size_t j = 0; j < face.mNumIndices; j++)
			newMesh.indices.push_back(face.mIndices[j]);
	}


	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		struct MaterialInfo
		{
			aiTextureType type;
			std::string   typeName;
			std::string   defaultTextureFile;
			VkFormat      format;
			int           desiredChannels;
		};

		std::vector<MaterialInfo> materials =
		{
			{ aiTextureType_DIFFUSE,	"DIFFUSE",				"textures/default/DefaultTexture.png",		VK_FORMAT_R8G8B8A8_SRGB,	4},
			{ aiTextureType_UNKNOWN,	"METALIC_ROUGHNESS",	"textures/default/metallicRoughness.png",	VK_FORMAT_R8G8B8A8_SRGB,	4},
			{ aiTextureType_EMISSIVE,	"EMISSIVE",				"textures/default/emissiveColor.png",		VK_FORMAT_R8G8B8A8_SRGB,	4},
			{ aiTextureType_LIGHTMAP,	"AO",					"textures/default/ambientOcclusion.png",	VK_FORMAT_R8G8B8A8_SRGB,	4},
			{ aiTextureType_NORMALS,	"NORMALS",				"textures/default/DefaultNormal.png",		VK_FORMAT_R8G8B8A8_SRGB,	4}
		};

		TextureToLoadInfo info;
		for (auto& m : materials)
		{
			info.name = getMaterialTextureName(material, m.type, m.typeName, m.defaultTextureFile);
			info.format = m.format;
			info.desiredChannels = m.desiredChannels;

			newMesh.texturesToLoadInfo.push_back(info);
		}
	}
	m_meshes.push_back(newMesh);
}

void NormalPBR::createUniformBuffers(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice,const uint32_t& uboCount) 
{
	m_ubo = std::make_shared<UBO>(physicalDevice, logicalDevice, uboCount, sizeof(DescriptorTypes::UniformBufferObject::NormalPBR));
	m_uboLights = std::make_shared<UBO>(physicalDevice, logicalDevice, uboCount, sizeof(DescriptorTypes::UniformBufferObject::LightInfo) * 10);
}

void NormalPBR::bindData(const Graphics* graphicsPipeline,const VkCommandBuffer& commandBuffer,const uint32_t currentFrame) 
{
	for (auto& mesh : m_meshes)
	{
		CommandManager::STATE::bindVertexBuffers({ mesh.vertexBuffer }, { 0 }, 0, 1, commandBuffer);
		CommandManager::STATE::bindIndexBuffer(mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32, commandBuffer);

		CommandManager::STATE::bindDescriptorSets(graphicsPipeline->getPipelineLayout(), PipelineType::GRAPHICS, 0, { mesh.descriptorSets.get(currentFrame) }, {}, commandBuffer);

		CommandManager::ACTION::drawIndexed(mesh.indices.size(), 1, 0, 0, 0, commandBuffer);
	}
}


void NormalPBR::createDescriptorSets(const VkDevice& logicalDevice,const VkDescriptorSetLayout& descriptorSetLayout, DescriptorSetInfo* info, DescriptorPool& descriptorPool)
{
	std::vector<UBO*> opUBOs = { (m_ubo.get()),(m_uboLights.get()) };

	for (auto& mesh : m_meshes)
	{
		mesh.descriptorSets = DescriptorSets(
			logicalDevice,
			GRAPHICS_PIPELINE::PBR::UBOS_INFO,
			GRAPHICS_PIPELINE::PBR::SAMPLERS_INFO,
			mesh.textures,
			opUBOs,
			descriptorSetLayout,
			descriptorPool,
			// TODO: Improve this
			info
		);
	}
}

void NormalPBR::uploadVertexData(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice, const VkQueue& graphicsQueue, const std::shared_ptr<CommandPool>& commandPool)
{

	for (auto& mesh : m_meshes)
	{
		// Vertex Buffer(with staging buffer)
		BufferManager::createBufferAndTransferToDevice(
			commandPool,
			physicalDevice,
			logicalDevice,
			mesh.vertices.data(),
			sizeof(mesh.vertices[0]) * mesh.vertices.size(),
			graphicsQueue,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			mesh.vertexMemory,
			mesh.vertexBuffer
		);

		// Index Buffer(with staging buffer)
		BufferManager::createBufferAndTransferToDevice(
			commandPool,
			physicalDevice,
			logicalDevice,
			mesh.indices.data(),
			sizeof(mesh.indices[0]) * mesh.indices.size(),
			graphicsQueue,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			mesh.indexMemory,
			mesh.indexBuffer
		);
	}
}

/*
 * Creates and loads all the samplers used in the shader of each mesh.
 */
void NormalPBR::uploadTextures(const VkPhysicalDevice& physicalDevice,const VkDevice& logicalDevice, const VkSampleCountFlagBits& samplesCount, const std::shared_ptr<CommandPool>& commandPool, const VkQueue& graphicsQueue)
{
	const size_t nTextures = GRAPHICS_PIPELINE::PBR::TEXTURES_PER_MESH_COUNT;

	for (auto& mesh : m_meshes)
	{
		// Samplers of textures.
		for (size_t i = 0; i < nTextures; i++)
		{
			auto it = (m_texturesID.find(mesh.texturesToLoadInfo[i].name));

			if (it == m_texturesID.end())
			{
				mesh.textures.push_back(std::make_shared<NormalTexture>(physicalDevice,logicalDevice, mesh.texturesToLoadInfo[i],samplesCount,commandPool,graphicsQueue, UsageType::TO_COLOR));
				m_texturesLoaded.push_back(mesh.textures[i]);
				m_texturesID[mesh.texturesToLoadInfo[i].name] = (m_texturesLoaded.size() - 1);
			}
			else
				mesh.textures.push_back(m_texturesLoaded[it->second]);
		}
	}
}

const glm::mat4& NormalPBR::getModelM() const
{
	return m_dataInShader.model;
}

void NormalPBR::updateUBO(
	const VkDevice&				logicalDevice,
	const uint32_t&				currentFrame,
	const UBOinfo&				uboInfo
) {

	m_dataInShader.model = MathUtils::getUpdatedModelMatrix(m_pos, m_rot, m_size);
	m_dataInShader.view = uboInfo.view;
	m_dataInShader.proj = uboInfo.proj;
	m_dataInShader.lightSpace = uboInfo.lightSpace;

	m_dataInShader.cameraPos = uboInfo.cameraPos;
	m_dataInShader.lightsCount = uboInfo.lightsCount;
	m_dataInShader.hasNormalMap = m_hasNormalMap;

	size_t size = sizeof(m_dataInShader);
	UBOutils::updateUBO(logicalDevice, m_ubo, size, &m_dataInShader, currentFrame);
}

void NormalPBR::updateUBOlights(
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
			m_lightsInfo[i].intensity = pModel->getIntensity();
			m_lightsInfo[i].type = (int)pModel->getLightType();
		}
	}
	size_t size = sizeof(m_lightsInfo[0]) * 10;

	UBOutils::updateUBO(logicalDevice,m_uboLights,size,&m_lightsInfo,currentFrame);
}


const std::vector<Mesh<Attributes::PBR::Vertex>>& NormalPBR::getMeshes() const
{
	return m_meshes;
}