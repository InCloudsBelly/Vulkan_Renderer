#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <cstddef>
#include <functional>

#include <vulkan/vulkan.h>
#include <VMA/vk_mem_alloc.h>

#include <glm/glm.hpp>

template<typename T>
inline void hash_combine(std::uint32_t& seed, const T& v)
{
    seed ^= std::hash<T> {}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T, typename... Ts>
inline void hash_combine(std::uint32_t& seed, const T& v, Ts... rest)
{
    hash_combine(seed, v);
    if constexpr (sizeof...(Ts) > 1)
    {
        hash_combine(seed, rest...);
    }
}



using GObjectID = std::uint32_t;
constexpr GObjectID k_invalid_gobject_id = std::numeric_limits<std::uint32_t>::max();
constexpr uint32_t k_invalid_part_id = std::numeric_limits<uint32_t>::max();


struct GameObjectPartId
{
    GObjectID m_go_id{ k_invalid_gobject_id };
    uint32_t    m_part_id{ k_invalid_part_id };

    bool   operator==(const GameObjectPartId& rhs) const { return m_go_id == rhs.m_go_id && m_part_id == rhs.m_part_id; }
    uint32_t getHashValue() const { return m_go_id ^ (m_part_id << 1); }
    bool   isValid() const { return m_go_id != k_invalid_gobject_id && m_part_id != k_invalid_part_id; }
};


struct MeshSourceDesc
{
    std::string m_mesh_file;

    bool   operator==(const MeshSourceDesc& rhs) const { return m_mesh_file == rhs.m_mesh_file; }
    uint32_t getHashValue() const { return std::hash<std::string> {}(m_mesh_file); }
};


struct MaterialSourceDesc
{
    std::string m_base_color_file;
    std::string m_metallic_roughness_file;
    std::string m_normal_file;
    std::string m_occlusion_file;
    std::string m_emissive_file;

    bool operator==(const MaterialSourceDesc& rhs) const
    {
        return m_base_color_file == rhs.m_base_color_file &&
            m_metallic_roughness_file == rhs.m_metallic_roughness_file &&
            m_normal_file == rhs.m_normal_file &&
            m_occlusion_file == rhs.m_occlusion_file &&
            m_emissive_file == rhs.m_emissive_file;
    }

    uint32_t getHashValue() const
    {
        uint32_t hash = 0;
        hash_combine(hash,
            m_base_color_file,
            m_metallic_roughness_file,
            m_normal_file,
            m_occlusion_file,
            m_emissive_file);
        return hash;
    }
};


class BufferData
{
public:
    uint32_t m_size{ 0 };
    void* m_data{ nullptr };

    BufferData() = delete;
    BufferData(uint32_t size)
    {
        m_size = size;
        m_data = malloc(size);
    }
    ~BufferData()
    {
        if (m_data)
        {
            free(m_data);
        }
    }
    bool isValid() const { return m_data != nullptr; }
};

struct StaticMeshData
{
    std::shared_ptr<BufferData> m_vertex_buffer;
    std::shared_ptr<BufferData> m_index_buffer;

    uint32_t            m_meshVertexCount;
    uint32_t            m_meshIndexCount;
};

struct MeshVertex
{
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;
};


struct TextureToLoadInfo
{
    std::string name;
    std::string folderName;
    VkFormat    format;
    int         desiredChannels;
};

struct MaterialDataInfo
{
    std::vector<TextureToLoadInfo> info;
};

