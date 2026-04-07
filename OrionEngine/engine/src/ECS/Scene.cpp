#include "ECS/Scene.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>


namespace Orion {

    const std::vector<EntityID> Scene::s_EmptyChildren = {};

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

        // Detach from parent if any
        RemoveParent(entityID);

        // Reparent children to root (detach them)
        auto relIt = m_Relationships.find(entityID);
        if (relIt != m_Relationships.end()) {
            // Copy children list since RemoveParent modifies it
            std::vector<EntityID> children = relIt->second.children;
            for (EntityID child : children)
                RemoveParent(child);
        }
        m_Relationships.erase(entityID);

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
        m_CameraComponents.erase(entityID);
        m_ScriptComponents.erase(entityID);
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
        m_CameraComponents.clear();
        m_ScriptComponents.clear();
        m_Relationships.clear();
    }

    std::shared_ptr<Scene> Scene::Copy() const
    {
        auto newScene = std::make_shared<Scene>();
        newScene->m_NextEntityID = m_NextEntityID;
        newScene->m_Entities = m_Entities;
        newScene->m_EntityDataComponents = m_EntityDataComponents;
        newScene->m_TransformComponents = m_TransformComponents;
        newScene->m_MeshComponents = m_MeshComponents;
        newScene->m_MaterialComponents = m_MaterialComponents;
        newScene->m_CameraComponents = m_CameraComponents;
        newScene->m_ScriptComponents = m_ScriptComponents;
        newScene->m_Relationships = m_Relationships;
        return newScene;
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

    void Scene::RemoveMeshComponent(EntityID entityID)
    {
        m_MeshComponents.erase(entityID);
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

    void Scene::RemoveMaterialComponent(EntityID entityID)
    {
        m_MaterialComponents.erase(entityID);
    }


    // --- Camera ---
    void Scene::AddCameraComponent(EntityID entityID, const CameraComponent& component)
    {
        m_CameraComponents[entityID] = component;
    }

    CameraComponent* Scene::GetCameraComponent(EntityID entityID)
    {
        auto it = m_CameraComponents.find(entityID);
        if (it != m_CameraComponents.end())
            return &it->second;

        return nullptr;
    }

    bool Scene::HasCameraComponent(EntityID entityID) const
    {
        return m_CameraComponents.find(entityID) != m_CameraComponents.end();
    }

    void Scene::RemoveCameraComponent(EntityID entityID)
    {
        m_CameraComponents.erase(entityID);
    }


    // --- Script ---
    void Scene::AddScriptComponent(EntityID entityID, const ScriptComponent& component)
    {
        m_ScriptComponents[entityID] = component;
    }

    ScriptComponent* Scene::GetScriptComponent(EntityID entityID)
    {
        auto it = m_ScriptComponents.find(entityID);
        if (it != m_ScriptComponents.end())
            return &it->second;

        return nullptr;
    }

    bool Scene::HasScriptComponent(EntityID entityID) const
    {
        return m_ScriptComponents.find(entityID) != m_ScriptComponents.end();
    }

    void Scene::RemoveScriptComponent(EntityID entityID)
    {
        m_ScriptComponents.erase(entityID);
    }


    // --- Relationships (parent-child hierarchy) ---

    void Scene::SetParent(EntityID child, EntityID parent)
    {
        if (child == INVALID_ENTITY || parent == INVALID_ENTITY || child == parent)
            return;

        // Prevent circular parenting: walk up from 'parent' to make sure 'child' isn't an ancestor
        EntityID walk = parent;
        while (walk != INVALID_ENTITY) {
            if (walk == child)
                return; // Would create a cycle
            auto it = m_Relationships.find(walk);
            walk = (it != m_Relationships.end()) ? it->second.parent : INVALID_ENTITY;
        }

        // Detach from current parent first
        RemoveParent(child);

        // Set new parent
        m_Relationships[child].parent = parent;
        m_Relationships[parent].children.push_back(child);
    }

    void Scene::RemoveParent(EntityID child)
    {
        auto childIt = m_Relationships.find(child);
        if (childIt == m_Relationships.end() || childIt->second.parent == INVALID_ENTITY)
            return;

        EntityID oldParent = childIt->second.parent;
        childIt->second.parent = INVALID_ENTITY;

        // Remove from old parent's children list
        auto parentIt = m_Relationships.find(oldParent);
        if (parentIt != m_Relationships.end()) {
            auto& siblings = parentIt->second.children;
            siblings.erase(
                std::remove(siblings.begin(), siblings.end(), child),
                siblings.end()
            );
        }
    }

    EntityID Scene::GetParent(EntityID entityID) const
    {
        auto it = m_Relationships.find(entityID);
        if (it != m_Relationships.end())
            return it->second.parent;
        return INVALID_ENTITY;
    }

    const std::vector<EntityID>& Scene::GetChildren(EntityID entityID) const
    {
        auto it = m_Relationships.find(entityID);
        if (it != m_Relationships.end())
            return it->second.children;
        return s_EmptyChildren;
    }

    bool Scene::HasChildren(EntityID entityID) const
    {
        auto it = m_Relationships.find(entityID);
        return it != m_Relationships.end() && !it->second.children.empty();
    }

    bool Scene::HasParent(EntityID entityID) const
    {
        auto it = m_Relationships.find(entityID);
        return it != m_Relationships.end() && it->second.parent != INVALID_ENTITY;
    }

    std::vector<EntityID> Scene::GetRootEntities() const
    {
        std::vector<EntityID> roots;
        for (EntityID entity : m_Entities) {
            if (!HasParent(entity))
                roots.push_back(entity);
        }
        return roots;
    }


    // --- World transform ---

    glm::mat4 Scene::BuildLocalTransform(const TransformComponent& tc)
    {
        glm::mat4 transform(1.0f);
        transform = glm::translate(transform, tc.position);
        if (tc.rotation.x != 0.0f)
            transform = glm::rotate(transform, tc.rotation.x, glm::vec3(1, 0, 0));
        if (tc.rotation.y != 0.0f)
            transform = glm::rotate(transform, tc.rotation.y, glm::vec3(0, 1, 0));
        if (tc.rotation.z != 0.0f)
            transform = glm::rotate(transform, tc.rotation.z, glm::vec3(0, 0, 1));
        transform = glm::scale(transform, tc.scale);
        return transform;
    }

    glm::mat4 Scene::GetWorldTransform(EntityID entityID) const
    {
        // Walk up the parent chain, collecting local transforms on a stack,
        // then multiply top-down: root * ... * parent * child.
        std::vector<EntityID> chain;
        EntityID current = entityID;
        while (current != INVALID_ENTITY) {
            chain.push_back(current);
            current = GetParent(current);
        }

        glm::mat4 world(1.0f);
        // Multiply from root (back of chain) down to the entity (front)
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            auto tcIt = m_TransformComponents.find(*it);
            if (tcIt != m_TransformComponents.end())
                world = world * BuildLocalTransform(tcIt->second);
        }
        return world;
    }
}