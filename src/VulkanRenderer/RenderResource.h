#pragma once

#include <VMA/vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <map>

#include "VulkanRenderer/Texture/Texture.h"
#include "VulkanRenderer/Model/Model.h"

struct IBLResource
{
    std::shared_ptr<TextureBase> brdfLUT;
    std::shared_ptr<TextureBase> irradiance;
    std::shared_ptr<TextureBase> prefiltered_Env;
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
    uint32_t            mesh_vertex_count;

    VkBuffer*           meshVertexPositionBuffer;
    VmaAllocation       meshVertexPositionAllocation;

    VkBuffer*           vertexBuffer;
    VkBuffer*           indexBuffer;
    VmaAllocation	    vertexAllocation;
    VmaAllocation       indexAllocation;

    VkDescriptorSet*    meshDescriptorSet;

};

struct MaterialInfo
{
    std::shared_ptr<TextureBase> colorTexture;
    std::shared_ptr<TextureBase> metallic_RoughnessTexture;
    std::shared_ptr<TextureBase> normalTexture;
    std::shared_ptr<TextureBase> AOTexture;
    std::shared_ptr<TextureBase> emissiveTexture;
    
    VkBuffer*       materialUBO;
    VmaAllocation   materialUBOAllocation;

    VkDescriptorSet* materialDescriptorSet;

};


class RenderResource
{
public:
    RenderResource() {};

    void clear();

    void updateIBLResource(std::shared_ptr<TextureBase> brdfLUT, std::shared_ptr<TextureBase> Irradiance, std::shared_ptr<TextureBase> Env);

public:

    IBLResource                                     m_IBLResource;

    MeshPerframeStorageBufferObject                 m_PerframeResource;
    //// cached mesh and material
    //std::map<size_t, MeshInfo>                      m_vulkan_meshes;
    //std::map<size_t, MaterialInfo>                  m_vulkan_pbr_materials;

    std::vector<std::shared_ptr<Model>>             m_modelResource;
    size_t                                          m_skyboxIndex = -1;
    std::vector<size_t>                             m_objectModelIndices;
    std::vector<size_t>                             m_lightModelIndices;
    size_t                                          m_directionalLightIndex = -1;

    std::vector<Mesh<Attributes::SKYBOX::Vertex>>   m_skyboxMeshes;
    std::vector<Mesh<Attributes::PBR::Vertex>>      m_normalMeshes;
    std::vector<Mesh<Attributes::LIGHT::Vertex>>    m_lightMeshes;


    VkDescriptorSetLayout* const* mMeshDescriptorSetLayout{ nullptr };
    VkDescriptorSetLayout* const* mMaterialDescriptorSetLayout{ nullptr };

};