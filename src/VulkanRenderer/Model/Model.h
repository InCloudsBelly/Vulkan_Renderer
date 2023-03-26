#pragma once

#include <vector>
#include <array>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


#include "VulkanRenderer/Buffer/BufferUtils.h"
#include "VulkanRenderer/Model/Mesh.h"
#include "VulkanRenderer/Texture/Texture.h"

#include "VulkanRenderer/Features/ShadowMap.h"

enum class ModelType
{
	NORMAL_PBR = 1,
	LIGHT = 2,
	SKYBOX = 3
};


class Model
{
public:
	Model(
		const std::string& name, const std::string& folderName,
		const ModelType& type,
		const glm::fvec4& pos = glm::fvec4(0.0f),
		const glm::fvec3& rot = glm::fvec3(0.0f),
		const glm::fvec3& size = glm::fvec3(1.0f));

	virtual ~Model() = 0;
	virtual void destroy() = 0;

	void upload(
		const VkQueue& graphicsQueue,
		const VkCommandPool& commandPool,
		const uint32_t						uboCount
	);

	virtual void bindData(
		const Graphics* graphicsPipeline,
		const VkCommandBuffer& commandBUffer
	) = 0;

	virtual void createDescriptorSets(
		const VkDescriptorSetLayout& descriptorSetLayout,
		std::vector<VkDescriptorImageInfo*> info,
		VkDescriptorPool& descriptorPool
	) = 0;

	virtual void updateUBO(
		const uint32_t& currentFrame,
		const UBOinfo& uboInfo
	) = 0;

	const std::string& getName() const;
	const ModelType& getType() const;
	const glm::fvec4& getPos() const;
	const glm::fvec3& getRot() const;
	const glm::fvec3& getSize() const;
	const bool isHidden() const;
	void setPos(const glm::fvec4& newPos);
	void setRot(const glm::fvec3& newRot);
	void setSize(const glm::fvec3& newSize);
	void setHideStatus(const bool status);

protected:
	virtual void processMesh(aiMesh* mesh, const aiScene* scene) = 0;
	void loadModel(const char* pathToModel);

	virtual void uploadVertexData(
		const VkQueue& graphicsQueue,
		const VkCommandPool& commandPool
	) = 0;
	virtual void uploadTextures(
		const VkSampleCountFlagBits& samplesCount,
		const VkCommandPool& commandPool,
		const VkQueue& graphicsQueue
	) = 0;
	virtual void createUniformBuffers(
		const uint32_t& uboCount
	) = 0;

	ModelType            m_type;
	std::string          m_name;
	std::string          m_folderName;

	VkBuffer			 m_ubo;
	VmaAllocation		 m_uboAllocation;

	glm::fvec4           m_pos;
	glm::fvec3           m_rot;
	glm::fvec3           m_size;

	bool                 m_hideStatus;

	// We need these to not reload the textures again if they are used multiple
	// times in different(or in the same) meshes.

	std::vector<std::shared_ptr<TextureBase>> m_texturesLoaded;

	std::unordered_map<std::string, size_t> m_texturesID;
private:
	void processNode(aiNode* node, const aiScene* scene);
};