#include "VulkanRenderer/Model/Types/NormalPBR.h"

#include <iostream>


#include "VulkanRenderer/Settings/GraphicsPipelineConfig.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Model/Types/Light.h"
#include "VulkanRenderer/Math/MathUtils.h"
#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Command/CommandManager.h"
#include "VulkanRenderer/Descriptor/DescriptorManager.h"

#include "VulkanRenderer/Renderer.h"

NormalPBR::NormalPBR(const ModelInfo& modelInfo)
	: Model(modelInfo.name, modelInfo.folderName, ModelType::NORMAL_PBR, glm::fvec4(modelInfo.pos, 1.0f), modelInfo.rot, modelInfo.size)
{
	loadModel((std::string(MODEL_DIR) + modelInfo.folderName + "/" + modelInfo.fileName).c_str());
}

NormalPBR::~NormalPBR() {}

void NormalPBR::destroy()
{
	vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_ubo, m_uboAllocation);
	vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_uboLight, m_uboLightAllocation);

	for (auto& texture : m_texturesLoaded)
		texture->destroy();

	for (auto& mesh : m_meshes)
	{
		vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), mesh.vertexBuffer, mesh.vertexAllocation);
		vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), mesh.indexBuffer, mesh.indexAllocation);
	}
}

void NormalPBR::getMaterialTextureInfo(aiMaterial* material, const aiTextureType& type, const std::string& typeName, const std::string& defaultTextureFile, TextureToLoadInfo& info)
{
	if (material->GetTextureCount(type) > 0)
	{
		aiString str;
		material->GetTexture(type, 0, &str);

		if (typeName == "NORMALS")
			m_dataInShader.hasNormalMap = 1;

		if (typeName == "METALIC_ROUGHNESS")
			m_dataInShader.hasMetallicRoughnessMap = 1;

		info.folderName = m_folderName;
		info.name = str.C_Str();
	}
	else
	{
		if (typeName == "NORMALS")
			m_dataInShader.hasNormalMap = 0;

		if (typeName == "METALIC_ROUGHNESS")
			m_dataInShader.hasMetallicRoughnessMap = 0;

		info.folderName = "/defaultTextures";

		info.name = defaultTextureFile;
	}

}

void NormalPBR::processMesh(aiMesh* mesh, const aiScene* scene)
{
	Mesh<Attributes::PBR::Vertex> newMesh;

	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		Attributes::PBR::Vertex vertex{};

		//Position
		vertex.pos = { mesh->mVertices[i].x,mesh->mVertices[i].y,mesh->mVertices[i].z };
		if (mesh->mNormals != NULL)
		{
			vertex.normal = glm::normalize(glm::fvec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
		}
		else
			throw std::runtime_error("Mesh doesn't have normals!");


		//TexCoords
		if (mesh->mTextureCoords[0] != NULL)
		{
			vertex.texCoord = { mesh->mTextureCoords[0][i].x,mesh->mTextureCoords[0][i].y, };
		}
		else
		{
			vertex.texCoord = glm::fvec3(1.0f);
		}

		//Tangents
		if (mesh->mTangents != NULL)
		{
			vertex.tangent = glm::normalize(glm::fvec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z));
		}
		else
			vertex.tangent = glm::fvec3(1.0f);

		//glm::vec3 bitangent = glm::cross(vertex.normal, tangent);

		vertex.posInLightSpace = glm::fvec4(1.0f);

		newMesh.vertices.emplace_back(vertex);
	}

	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		auto face = mesh->mFaces[i];
		for (size_t j = 0; j < face.mNumIndices; j++)
			newMesh.indices.emplace_back(face.mIndices[j]);
	}


	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// Roughness and metallic factor.
		aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &m_dataInShader.metallicFactor);
		aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &m_dataInShader.roughnessFactor);

		// Material Textures
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
			{ aiTextureType_DIFFUSE,	"DIFFUSE",				"DefaultTexture.png",		VK_FORMAT_R8G8B8A8_SRGB,	4},
			{ aiTextureType_UNKNOWN,	"METALIC_ROUGHNESS",	"metallicRoughness.png",	VK_FORMAT_R8G8B8A8_SRGB,	4},
			{ aiTextureType_EMISSIVE,	"EMISSIVE",				"emissiveColor.png",		VK_FORMAT_R8G8B8A8_SRGB,	4},
			{ aiTextureType_LIGHTMAP,	"AO",					"ambientOcclusion.png",		VK_FORMAT_R8G8B8A8_SRGB,	4},
			{ aiTextureType_NORMALS,	"NORMALS",				"DefaultNormal.png",		VK_FORMAT_R8G8B8A8_SRGB,	4}
		};

		TextureToLoadInfo info;
		for (auto& m : materials)
		{
			getMaterialTextureInfo(material, m.type, m.typeName, m.defaultTextureFile, info);
			info.format = m.format;
			info.desiredChannels = m.desiredChannels;

			newMesh.texturesToLoadInfo.emplace_back(info);
		}
	}
	m_meshes.emplace_back(newMesh);
}

void NormalPBR::createUniformBuffers( const uint32_t& uboCount)
{
	BufferManager::bufferCreateBuffer(
		getRendererPointer()->getVmaAllocator(),
		sizeof(DescriptorTypes::UniformBufferObject::NormalPBR),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&m_ubo,
		&m_uboAllocation
	);

	BufferManager::bufferCreateBuffer(
		getRendererPointer()->getVmaAllocator(),
		sizeof(DescriptorTypes::UniformBufferObject::LightInfo) * 10,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&m_uboLight,
		&m_uboLightAllocation
	);
}

void NormalPBR::bindData(const Graphics* graphicsPipeline, const VkCommandBuffer& commandBuffer)
{
	for (auto& mesh : m_meshes)
	{
		std::vector<VkBuffer> vertexBuffers = { mesh.vertexBuffer };
		std::vector<VkDeviceSize> offsets = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
		vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		const std::vector<VkDescriptorSet> sets = { mesh.descriptorSet };
		vkCmdBindDescriptorSets(
			commandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicsPipeline->getPipelineLayout(),
			// Index of the first descriptor set.
			0,
			sets.size(), sets.data(),
			0, {}
		);

		vkCmdDrawIndexed(commandBuffer, mesh.indices.size(), 1, 0, 0, 0);
	}
}


void NormalPBR::createDescriptorSets(const VkDescriptorSetLayout& descriptorSetLayout, std::vector<VkDescriptorImageInfo*> info, VkDescriptorPool& descriptorPool)
{
	for (auto& mesh : m_meshes)
	{
		DescriptorManager::allocDescriptorSet(descriptorPool, descriptorSetLayout, &mesh.descriptorSet);

		DescriptorManager::createDescriptorSet(
			GRAPHICS_PIPELINE::PBR::DESCRIPTORS_INFO,
			mesh.textures,
			info,
			{ (m_ubo),(m_uboLight) },
			&mesh.descriptorSet
		);
	}
}

void NormalPBR::uploadVertexData(const VkQueue& graphicsQueue, const VkCommandPool& commandPool)
{

	for (auto& mesh : m_meshes)
	{
		BufferManager::createBufferAndTransferToDevice(
			getRendererPointer()->getDevice(),
			getRendererPointer()->getVmaAllocator(),
			graphicsQueue,
			commandPool,
			mesh.vertices.data(),
			sizeof(mesh.vertices[0]) * mesh.vertices.size(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			&mesh.vertexBuffer,
			&mesh.vertexAllocation
		);

		BufferManager::createBufferAndTransferToDevice(
			getRendererPointer()->getDevice(),
			getRendererPointer()->getVmaAllocator(),
			graphicsQueue,
			commandPool,
			mesh.indices.data(),
			sizeof(mesh.indices[0]) * mesh.indices.size(),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			&mesh.indexBuffer,
			&mesh.indexAllocation
		);
	}
}

/*
 * Creates and loads all the samplers used in the shader of each mesh.
 */
void NormalPBR::uploadTextures(const VkSampleCountFlagBits& samplesCount, const VkCommandPool& commandPool, const VkQueue& graphicsQueue)
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
				mesh.textures.push_back(std::make_shared<NormalTexture>(
					mesh.texturesToLoadInfo[i].name,
					std::string(MODEL_DIR) + mesh.texturesToLoadInfo[i].folderName,
					mesh.texturesToLoadInfo[i].format
					)
				);

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
	const uint32_t& currentFrame,
	const UBOinfo& uboInfo
) {

	m_dataInShader.model = MathUtils::getUpdatedModelMatrix(m_pos, m_rot, m_size);
	m_dataInShader.view = uboInfo.view;
	m_dataInShader.proj = uboInfo.proj;
	m_dataInShader.lightSpace = uboInfo.lightSpace;

	m_dataInShader.cameraPos = uboInfo.cameraPos;
	m_dataInShader.lightsCount = uboInfo.lightsCount;

	void* data;
	vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocation, &data);
		memcpy(data, &m_dataInShader, sizeof(m_dataInShader));
	vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocation);
}

void NormalPBR::updateUBOlights(
	const uint32_t& currentFrame
) {
	for (size_t i = 0; i < getRenderResource()->m_lightModelIndices.size(); i++)
	{
		size_t j = getRenderResource()->m_lightModelIndices[i];

		if (auto pModel = std::dynamic_pointer_cast<Light>(getRenderResource()->m_modelResource[j]))
		{
			m_lightsInfo[i].pos = pModel->getPos();
			m_lightsInfo[i].color = pModel->getColor();
			m_lightsInfo[i].dir = pModel->getTargetPos() - pModel->getPos();
			m_lightsInfo[i].intensity = pModel->getIntensity();
			m_lightsInfo[i].type = (int)pModel->getLightType();
		}
	}

	void* data;
	vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboLightAllocation, &data);
		memcpy(data, &m_lightsInfo, sizeof(m_lightsInfo[0]) * 10);
	vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboLightAllocation);
}


const std::vector<Mesh<Attributes::PBR::Vertex>>& NormalPBR::getMeshes() const
{
	return m_meshes;
}