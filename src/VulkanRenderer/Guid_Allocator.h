#pragma once
#include <map>

static const uint32_t s_invalid_guid = 0;

template<typename T>
class GuidAllocator
{
public:
    GuidAllocator() {};

    static bool isValidGuid(uint32_t guid) { return guid != s_invalid_guid; }

    uint32_t allocGuid(const T& t)
    {
        auto find_it = m_elements_guid_map.find(t);
        if (find_it != m_elements_guid_map.end())
        {
            return find_it->second;
        }

        for (uint32_t i = 0; i < m_guid_elements_map.size() + 1; i++)
        {
            uint32_t guid = i + 1;
            if (m_guid_elements_map.find(guid) == m_guid_elements_map.end())
            {
                m_guid_elements_map.insert(std::make_pair(guid, t));
                m_elements_guid_map.insert(std::make_pair(t, guid));
                return guid;
            }
        }

        return s_invalid_guid;
    }

    bool getGuidRelatedElement(uint32_t guid, T& t)
    {
        auto find_it = m_guid_elements_map.find(guid);
        if (find_it != m_guid_elements_map.end())
        {
            t = find_it->second;
            return true;
        }
        return false;
    }

    bool getElementGuid(const T& t, uint32_t& guid)
    {
        auto find_it = m_elements_guid_map.find(t);
        if (find_it != m_elements_guid_map.end())
        {
            guid = find_it->second;
            return true;
        }
        return false;
    }

    bool hasElement(const T& t) { return m_elements_guid_map.find(t) != m_elements_guid_map.end(); }

    void freeGuid(uint32_t guid)
    {
        auto find_it = m_guid_elements_map.find(guid);
        if (find_it != m_guid_elements_map.end())
        {
            const auto& ele = find_it->second;
            m_elements_guid_map.erase(ele);
            m_guid_elements_map.erase(guid);
        }
    }

    void freeElement(const T& t)
    {
        auto find_it = m_elements_guid_map.find(t);
        if (find_it != m_elements_guid_map.end())
        {
            const auto& guid = find_it->second;
            m_elements_guid_map.erase(t);
            m_guid_elements_map.erase(guid);
        }
    }

    std::vector<uint32_t> getAllocatedGuids() const
    {
        std::vector<uint32_t> allocated_guids;
        for (const auto& ele : m_guid_elements_map)
        {
            allocated_guids.push_back(ele.first);
        }
        return allocated_guids;
    }

    void clear()
    {
        m_elements_guid_map.clear();
        m_guid_elements_map.clear();
    }

private:
    std::map<T, uint32_t> m_elements_guid_map;
    std::map<uint32_t, T> m_guid_elements_map;
};