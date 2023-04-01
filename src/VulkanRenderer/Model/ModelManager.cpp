#include "VulkanRenderer/Model/ModelManager.h"

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Buffer/BufferManager.h"

#include <stdexcept>

Model::Model(
    const std::string& name, const std::string& filename, const std::string& folderName,
    const ModelType& type,
    const glm::fvec4& pos,
    const glm::fvec3& rot,
    const glm::fvec3& size
) : m_name(name), m_fileName(filename), m_folderName(folderName), m_type(type), m_pos(pos), m_rot(rot), m_size(size), m_hideStatus(false)
{
    if (m_type == ModelType::SKYBOX)
        loadModel((std::string(MODEL_DIR) + "cubeDefault/Cube.gltf").c_str());
    else if (m_type == ModelType::NORMAL_PBR)
        loadModel((std::string(MODEL_DIR) + m_folderName + "/" + m_fileName).c_str());
    else if (m_type == ModelType::LIGHT)
    {
        if (getRenderResource()->m_lightSphericalMeshIndex != 0)
            m_meshIndices.push_back(getRenderResource()->m_lightSphericalMeshIndex);
        else
            loadModel((std::string(MODEL_DIR) + "lightSphereDefault" + "/" + "lightSphere.obj").c_str());
    }
}


void Model::loadModel(const char* pathToModel)
{
    unsigned int flags = (aiProcess_Triangulate | aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);

    Assimp::Importer importer;
    auto* scene = importer.ReadFile(pathToModel, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        throw std::runtime_error("ERROR::ASSIMP::" + std::string(importer.GetErrorString()));
    }
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // Processes all the node's meshes(if any).
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        uint32_t meshIndex = getRenderResource()->m_meshInfoMap.size() + 1;
        RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];
        renderMeshInfo.mesh_id = meshIndex;

        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshData[meshIndex] = processMesh(mesh, scene);

        if (m_type == ModelType::NORMAL_PBR)
            m_materialData[meshIndex] = processMaterial(mesh, scene);


        m_meshIndices.push_back(renderMeshInfo.mesh_id);


        if (m_type == ModelType::SKYBOX)
            getRenderResource()->m_defaultCubeMeshIndex = renderMeshInfo.mesh_id;
        if (m_type == ModelType::LIGHT)
            getRenderResource()->m_lightSphericalMeshIndex = renderMeshInfo.mesh_id;
    }

    // Processes all the node's childrens(if any).
    for (uint32_t i = 0; i < node->mNumChildren; i++)
        processNode(node->mChildren[i], scene);
}

StaticMeshData Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    StaticMeshData mesh_data;

    std::vector<MeshVertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;

    for (uint32_t i = 0; i < mesh->mNumVertices; i++)
    {
        MeshVertex vertex{};

        //Position
        vertex.pos = { mesh->mVertices[i].x,mesh->mVertices[i].y,mesh->mVertices[i].z };
        if (mesh->mNormals != NULL)
            vertex.normal = glm::normalize(glm::fvec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
        else
            throw std::runtime_error("Mesh doesn't have normals!");


        //TexCoords
        if (mesh->mTextureCoords[0] != NULL)
            vertex.texCoord = { mesh->mTextureCoords[0][i].x,mesh->mTextureCoords[0][i].y, };
        else
            vertex.texCoord = glm::fvec3(1.0f);

        //Tangents
        if (mesh->mTangents != NULL)
            vertex.tangent = glm::normalize(glm::fvec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z));
        else
            vertex.tangent = glm::fvec3(1.0f);

        mesh_vertices.emplace_back(vertex);
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        auto face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++)
            mesh_indices.emplace_back(face.mIndices[j]);
    }

    mesh_data.m_meshVertexCount = mesh_vertices.size();
    mesh_data.m_meshIndexCount = mesh_indices.size();

    uint32_t stride = sizeof(MeshVertex);
    mesh_data.m_vertex_buffer = std::make_shared<BufferData>(mesh_vertices.size() * stride);
    mesh_data.m_index_buffer = std::make_shared<BufferData>(mesh_indices.size() * sizeof(uint32_t));

    for (uint32_t i = 0; i < mesh_vertices.size(); i++)
        ((MeshVertex*)(mesh_data.m_vertex_buffer->m_data))[i] = mesh_vertices[i];

    for (uint32_t i = 0; i < mesh_indices.size(); i++)
        ((uint32_t*)(mesh_data.m_index_buffer->m_data))[i] = mesh_indices[i];

    return mesh_data;
}

MaterialDataInfo Model::processMaterial(aiMesh* mesh, const aiScene* scene)
{
    MaterialDataInfo ret;
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    // Roughness and metallic factor.

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
        if (material->GetTextureCount(m.type) > 0)
        {
            aiString str;
            material->GetTexture(m.type, 0, &str);
            info.folderName = m_folderName;
            info.name = str.C_Str();
        }
        else
        {
            info.folderName = "/defaultTextures";
            info.name = m.defaultTextureFile;
        }
        info.format = m.format;
        info.desiredChannels = m.desiredChannels;

        ret.info.emplace_back(info);
    }

    return ret;
}


void Model::upload(const VkCommandPool& commandPool, const VkQueue& graphicsQueue)
{

    for (uint32_t i = 0; i < m_meshIndices.size(); i++)
    {
        uint32_t  meshIndex = m_meshIndices[i];
        RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];


        //upload Mesh Data
        MeshInfo* meshInfo = renderMeshInfo.ref_mesh;

        BufferManager::createBufferAndTransferToDevice(
            getRendererPointer()->getDevice(),
            getRendererPointer()->getVmaAllocator(),
            graphicsQueue,
            commandPool,
            m_meshData[meshIndex].m_vertex_buffer->m_data,
            m_meshData[meshIndex].m_vertex_buffer->m_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            meshInfo->vertexBuffer,
            &meshInfo->vertexAllocation
        );

        BufferManager::createBufferAndTransferToDevice(
            getRendererPointer()->getDevice(),
            getRendererPointer()->getVmaAllocator(),
            graphicsQueue,
            commandPool,
            m_meshData[meshIndex].m_index_buffer->m_data,
            m_meshData[meshIndex].m_index_buffer->m_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            meshInfo->indexBuffer,
            &meshInfo->indexAllocation
        );

        meshInfo->meshVertexCount = m_meshData[meshIndex].m_meshVertexCount;
        meshInfo->meshIndexCount = m_meshData[meshIndex].m_meshIndexCount;

        m_meshData[meshIndex].m_index_buffer.reset();
        m_meshData[meshIndex].m_vertex_buffer.reset();


        //upload Material Data
        if (m_materialData.size() > 0)
        {
            renderMeshInfo.ref_material = new MaterialInfo;
            MaterialInfo* materialInfo = renderMeshInfo.ref_material;

            materialInfo->colorTexture = std::make_shared<NormalTexture>(m_materialData[meshIndex].info[0].name, std::string(MODEL_DIR) + m_materialData[meshIndex].info[0].folderName, m_materialData[meshIndex].info[0].format);
            materialInfo->metallic_RoughnessTexture = std::make_shared<NormalTexture>(m_materialData[meshIndex].info[1].name, std::string(MODEL_DIR) + m_materialData[meshIndex].info[1].folderName, m_materialData[meshIndex].info[1].format);
            materialInfo->emissiveTexture = std::make_shared<NormalTexture>(m_materialData[meshIndex].info[2].name, std::string(MODEL_DIR) + m_materialData[meshIndex].info[2].folderName, m_materialData[meshIndex].info[2].format);
            materialInfo->AOTexture = std::make_shared<NormalTexture>(m_materialData[meshIndex].info[3].name, std::string(MODEL_DIR) + m_materialData[meshIndex].info[3].folderName, m_materialData[meshIndex].info[3].format);
            materialInfo->normalTexture = std::make_shared<NormalTexture>(m_materialData[meshIndex].info[4].name, std::string(MODEL_DIR) + m_materialData[meshIndex].info[4].folderName, m_materialData[meshIndex].info[4].format);


            m_materialData[meshIndex].info.clear();
            m_materialData[meshIndex].info.shrink_to_fit();
        }
    }

    if (m_type == ModelType::SKYBOX)
    {
        TextureToLoadInfo info = { m_fileName, m_folderName, VK_FORMAT_R32G32B32A32_SFLOAT, 4 };
        getRenderResource()->m_skyboxCubeMap = (std::make_shared<CubeMapTexture>(info.name, info.folderName, info.format));
    }
    else if (m_type == ModelType::LIGHT)
    {
        const TextureToLoadInfo info = { "DefaultTexture.png", "defaultTextures",VK_FORMAT_R8G8B8A8_SRGB , 4 };
        getRenderResource()->m_defaultTexture = (std::make_shared<NormalTexture>(info.name, std::string(MODEL_DIR) + info.folderName, info.format));
    }

}

