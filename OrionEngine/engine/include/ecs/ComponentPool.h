#pragma once


template<typename T>
class ComponentPool {
public:
	bool Has(EntityID entity) const
	{
		return m_Data.find(entity) != m_Data.end();
	}

	T& Add(EntityID entity, const T& component = T{})
	{
		return m_Data[entity] = component;
	}

    void Remove(EntityID entity)
    {
        m_Data.erase(entity);
    }

    T* Get(EntityID entity)
    {
        auto it = m_Data.find(entity);
        if (it == m_Data.end())
            return nullptr;
        return &it->second;
    }

    // "for (const auto& [entityID, mesh] : m_Meshes.GetAll())"
    const std::unordered_map<EntityID, T>& GetAll() const
    {
        return m_Data;
    }

private:
    std::unordered_map<EntityID, T> m_Data;
};