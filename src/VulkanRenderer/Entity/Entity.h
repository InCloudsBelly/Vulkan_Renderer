#pragma once


#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

    class RenderEntity
    {
    public:
        //uint32_t  m_instance_id{ 0 };
        
        glm::mat4 m_model_matrix{ 1 };

        // mesh
        uint32_t                  m_mesh_id{ 0 };
        //bool                   m_enable_vertex_blending{ false };
        /*AxisAlignedBox         m_bounding_box;*/

        // material
        uint32_t      m_material_asset_id{ 0 };
        glm::vec4   m_base_color_factor{ 1.0f, 1.0f, 1.0f, 1.0f };
        float       m_metallic_factor{ 1.0f };
        float       m_roughness_factor{ 1.0f };
        float       m_normal_scale{ 1.0f };
        float       m_occlusion_strength{ 1.0f };
        glm::vec3   m_emissive_factor{ 0.0f, 0.0f, 0.0f };
    };
