#pragma once

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vulkan/vulkan.h> 

#include "VulkanRenderer/RenderDataTypes.h"

#include "VulkanRenderer/Math/MathUtils.h"

enum class ModelType
{
	NORMAL_PBR = 1,
	LIGHT = 2,
	SKYBOX = 3
};


enum class LightType
{
	DIRECTIONAL_LIGHT = 0,
	POINT_LIGHT = 1,
	SPOT_LIGHT = 2,
	NONE = 3
};

struct ModelInfo
{
	ModelType   type;
	std::string name;
	std::string folderName;
	std::string fileName;
	glm::fvec3  color;
	glm::fvec3  pos;
	glm::fvec3  rot;
	glm::fvec3  size;

	// For light models.
	LightType   lType;
	glm::fvec3  endPos;
};


class Model
{
public:
	Model(
		const std::string& name,
		const std::string& filename,
		const std::string& folderName,
		const ModelType& type,
		const glm::fvec3& pos = glm::fvec3(0.0f),
		const glm::fvec3& rot = glm::fvec3(0.0f),
		const glm::fvec3& size = glm::fvec3(1.0f)
	);

	~Model() {};

	void upload(const VkCommandPool& commandPool, const VkQueue& graphicsQueue);

	const std::string& getName() const { return m_name; };
	const ModelType& getType() const { return m_type; };
	const glm::fvec3& getPos() const { return m_pos; };
	const glm::fvec3& getRot() const { return m_rot; };
	const glm::fvec3& getSize() const { return m_size; };
	const std::vector<uint32_t>& getMeshIndices() { return m_meshIndices; };

	const bool isHidden() const { return m_hideStatus; };
	void setPos(const glm::fvec3& newPos) { m_pos = newPos; };
	void setRot(const glm::fvec3& newRot) { m_rot = newRot; };
	void setSize(const glm::fvec3& newSize) { m_size = newSize; };
	void setHideStatus(const bool status) { m_hideStatus = status; };

	const glm::mat4& getModelMatrix() const { return MathUtils::getUpdatedModelMatrix(m_pos, m_rot, m_size); }

private:
	void loadModel(const char* pathToModel);
	void processNode(aiNode* node, const aiScene* scene);
	StaticMeshData processMesh(aiMesh* mesh, const aiScene* scene);
	MaterialDataInfo processMaterial(aiMesh* mesh, const aiScene* scene);



	ModelType				m_type;
	std::string				m_name;
	std::string				m_fileName;
	std::string				m_folderName;

	glm::fvec3				m_pos;
	glm::fvec3				m_rot;
	glm::fvec3				m_size;


	// only used for Light obj, TODO : Move LightInfos to Resource Manaegr
	glm::fvec3				m_targetPos;
	glm::fvec3				m_color;
	float					m_intensity;
	LightType				m_lightType;


	bool					m_hideStatus;

	std::vector<uint32_t>		m_meshIndices;

	std::unordered_map<uint32_t, StaticMeshData>		m_meshData;
	std::unordered_map<uint32_t, MaterialDataInfo>		m_materialData;
};