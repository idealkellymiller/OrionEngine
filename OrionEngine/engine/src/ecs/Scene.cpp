#include "Scene.h"
#include <algorithm>


Scene::Scene()
{
}

Scene::~Scene()
{
}

// Entity management
EntityID Scene::CreateEntity()
{
    EntityID id = m_NextEntityID++;
	m_Entities.push_back(id);
	return id;
}

EntityID Scene::CreateEntityWithID(EntityID entityID)
{
    if (entityID == INVALID_ENTITY)
        return INVALID_ENTITY;

    m_Entities.push_back(entityID);

    if (entityID >= m_NextEntityID)
        m_NextEntityID = entityID + 1;

    return entityID;
}

void Scene::DestroyEntity(EntityID entityID)
{
    if (!IsValidEntity(entityID))
        return;

    // Remove from entity list
    m_Entities.erase(
        std::remove(m_Entities.begin(), m_Entities.end(), entityID),
        m_Entities.end()
    );

    // Remove all components owned by this entity
    m_EntityDataComponents.erase(entityID);
    m_TransformComponents.erase(entityID);
    m_MeshComponents.erase(entityID);
    m_MaterialComponents.erase(entityID);
}

bool Scene::IsValidEntity(EntityID entityID) const
{
    if (entityID == INVALID_ENTITY)
        return false;

    return std::find(m_Entities.begin(), m_Entities.end(), entityID) != m_Entities.end();

}

void Scene::Clear()
{
    m_NextEntityID = 1;
    m_Entities.clear();
    m_EntityDataComponents.clear();
    m_MaterialComponents.clear();
    m_MeshComponents.clear();
    m_TransformComponents.clear();
}



// --- Entity Data ---
void Scene::AddEntityDataComponent(EntityID entityID, const EntityDataComponent& component)
{
    m_EntityDataComponents[entityID] = component;
}

EntityDataComponent* Scene::GetEntityDataComponent(EntityID entityID)
{
    auto it = m_EntityDataComponents.find(entityID);
    if (it != m_EntityDataComponents.end())
        return &it->second;

    return nullptr;
}

bool Scene::HasEntityDataComponent(EntityID entityID) const
{
    return m_EntityDataComponents.find(entityID) != m_EntityDataComponents.end();
}


// --- Transform ---
void Scene::AddTransformComponent(EntityID entityID, const TransformComponent& component)
{
    m_TransformComponents[entityID] = component;
}

TransformComponent* Scene::GetTransformComponent(EntityID entityID)
{
    auto it = m_TransformComponents.find(entityID);
    if (it != m_TransformComponents.end())
        return &it->second;

    return nullptr;
}

bool Scene::HasTransformComponent(EntityID entityID) const
{
    return m_TransformComponents.find(entityID) != m_TransformComponents.end();
}


// --- Mesh ---
void Scene::AddMeshComponent(EntityID entityID, const MeshComponent& component)
{
    m_MeshComponents[entityID] = component;
}

MeshComponent* Scene::GetMeshComponent(EntityID entityID)
{
    auto it = m_MeshComponents.find(entityID);
    if (it != m_MeshComponents.end())
        return &it->second;

    return nullptr;
}

bool Scene::HasMeshComponent(EntityID entityID) const
{
    return m_MeshComponents.find(entityID) != m_MeshComponents.end();
}


// ---- Material ---
void Scene::AddMaterialComponent(EntityID entityID, const MaterialComponent& component)
{
    m_MaterialComponents[entityID] = component;
}

MaterialComponent* Scene::GetMaterialComponent(EntityID entityID)
{
    auto it = m_MaterialComponents.find(entityID);
    if (it != m_MaterialComponents.end())
        return &it->second;

    return nullptr;
}

bool Scene::HasMaterialComponent(EntityID entityID) const
{
    return m_MaterialComponents.find(entityID) != m_MaterialComponents.end();
}
