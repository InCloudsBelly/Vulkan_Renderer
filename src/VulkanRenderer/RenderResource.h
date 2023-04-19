#pragma once

#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <map>
#include <array>
#include <string>
#include <unordered_map>

#include "VulkanRenderer/Model/ModelManager.h"

#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Guid_Allocator.h"
#include "VulkanRenderer/RenderDataTypes.h"
#include "VulkanRenderer/Settings/Config.h"

#include "VulkanRenderer/Features/PreIrradiance.h"
#include "VulkanRenderer/Features/PrefilteredEnvMap.h"

#include "VulkanRenderer/Camera/Camera.h"

struct IBLResource
{
    std::shared_ptr<TextureBase> brdfLUT;
    std::shared_ptr<TextureBase> irradiance;
    std::shared_ptr<TextureBase> prefiltered_Env;
};

struct UniformBuffer
{
    VkBuffer*       uboBuffer = new VkBuffer;
    VmaAllocation   uboAllocation;
};

struct MeshPerframeStorageLight
{
    glm::vec4   position;
    glm::vec4   dir;
    glm::vec4   color;
    float       attenuation;
    float       intensity;
    int         type;
};

struct MeshPerframeStorageBufferObject
{
    glm::mat4                   proj;
    glm::mat4                   view;
    glm::mat4                   lightSpace;
    glm::vec4                   cameraPos;
    int                         lightsCount;
    MeshPerframeStorageLight    sceneLights[10];
};


struct MeshInfo
{
    VkBuffer*           vertexBuffer = new VkBuffer;
    VkBuffer*           indexBuffer = new VkBuffer;
    VmaAllocation	    vertexAllocation;
    VmaAllocation       indexAllocation;

    uint32_t            meshVertexCount;
    uint32_t            meshIndexCount;
};

struct MaterialInfo
{
    std::shared_ptr<TextureBase> colorTexture;
    std::shared_ptr<TextureBase> metallic_RoughnessTexture;
    std::shared_ptr<TextureBase> emissiveTexture;
    std::shared_ptr<TextureBase> AOTexture;
    std::shared_ptr<TextureBase> normalTexture;
    
    //VkBuffer*       materialUBO;
    //VmaAllocation   materialUBOAllocation;

    //VkDescriptorSet* materialDescriptorSet;

};

// nodes
struct RenderMeshInfo
{
    uint32_t            mesh_id{0};

    // Vertex
    std::vector<MeshVertex>                vertices{};
    std::vector<uint32_t>                  indices{};

    StaticMeshData      meshData;

    const glm::mat4*    model_matrix = new glm::mat4(1);

    MeshInfo*           ref_mesh = new MeshInfo;
    MaterialInfo*       ref_material = nullptr;

    VkDescriptorSet*    descriptorSet = new VkDescriptorSet;

    bool operator <(const RenderMeshInfo a) const
    {
        return mesh_id < a.mesh_id ;
    }
};


struct LightInfo
{
    std::string             name;
    glm::fvec3              pos;
    glm::fvec3              rot;
    glm::fvec3              size;
    glm::fvec3				m_targetPos;
    glm::fvec3				m_color;
    float					m_intensity;
    LightType				m_lightType;
    std::shared_ptr<Model>  modelPtr;
};


class RenderResource
{
public:
    RenderResource() {};

    void loadModels(const std::vector<ModelInfo>& modelsToLoadInfo);
    void loadModel(const uint32_t startI, const uint32_t chunckSize, const std::vector<ModelInfo>& modelsToLoadInfo);
    void uploadModels(const VkQueue& graphicsQueue, const VkCommandPool& commandPool);

    void updateIBLResource(std::shared_ptr<TextureBase> brdfLUT, std::shared_ptr<TextureBase> Irradiance, std::shared_ptr<TextureBase> Env);

    void destroy();

public:

    uint32_t                                            staging_index = 1;

    IBLResource                                         m_IBLResource;

    Camera                                              m_camera;

    std::unordered_map<uint32_t, RenderMeshInfo>        m_meshInfoMap;
    std::shared_ptr<TextureBase>                        m_skyboxCubeMap;
    std::shared_ptr<TextureBase>                        m_defaultTexture;

    uint32_t                                            m_defaultCubeMeshIndex = 0;
    uint32_t                                            m_lightSphericalMeshIndex = 0;

    std::vector<std::shared_ptr<Model>>			        m_normalModels;
    std::shared_ptr<Model>			                    m_skybox;
    std::vector<std::shared_ptr<Model>>                 m_lightModels;

    std::vector<LightInfo>                            m_lightsInfo;
    uint32_t                                          m_directionalLightIndex;

    //VkDescriptorSetLayout* const* mMeshDescriptorSetLayout{ nullptr };
    //VkDescriptorSetLayout* const* mMaterialDescriptorSetLayout{ nullptr };


    // Global Features
    std::shared_ptr<PrefilteredIrradiance>             m_prefilteredIrradiance;
    std::shared_ptr<PrefilteredEnvMap>                 m_prefilteredEnv;

    //SH
    std::shared_ptr<NormalTexture>			            m_SHBRDFlut;
    glm::vec3                                           m_coefficient[Config::SH_COEF_NUM];
};