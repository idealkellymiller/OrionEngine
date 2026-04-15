#include "ECS/Scene.h"
#include <algorithm>
#include <cctype>
#include <unordered_set>
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
        m_RigidbodyComponents.erase(entityID);
        m_ColliderComponents.erase(entityID);
        m_PointLightComponents.erase(entityID);
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
        m_RigidbodyComponents.clear();
        m_ColliderComponents.clear();
        m_PointLightComponents.clear();
        m_Relationships.clear();
    }

    // Strip a trailing " (N)" suffix from `name` (where N is one or more digits),
    // returning just the base portion. If no such suffix is present, returns `name` unchanged.
    // Example: "Cube (3)" -> "Cube", "Cube" -> "Cube", "Cube (abc)" -> "Cube (abc)".
    static std::string StripCopySuffix(const std::string& name)
    {
        if (name.size() < 4 || name.back() != ')')
            return name;

        // Walk back from the closing ')' over digits.
        size_t i = name.size() - 2;
        size_t digitEnd = i;
        while (i != std::string::npos && std::isdigit(static_cast<unsigned char>(name[i])))
            --i;

        // Need at least one digit, and the char before the digits must be " (".
        if (i == std::string::npos || i == digitEnd) return name;  // no digits
        if (i < 1 || name[i] != '(' || name[i - 1] != ' ') return name;

        return name.substr(0, i - 1);
    }

    EntityID Scene::DuplicateEntity(EntityID source, bool duplicateChildren)
    {
        if (!IsValidEntity(source))
            return INVALID_ENTITY;

        EntityID newEntity = CreateEntity();

        // Copy every component the source has.
        if (HasEntityDataComponent(source)) {
            EntityDataComponent data = *GetEntityDataComponent(source);

            // Name the duplicate "base (N)", where base strips any existing " (N)" suffix
            // and N is the lowest positive integer not already in use by another entity.
            // This keeps "Cube" -> "Cube (1)" -> "Cube (2)" instead of stacking " (Copy) (Copy)".
            std::string base = StripCopySuffix(data.name);
            std::unordered_set<std::string> takenNames;
            for (const auto& [id, d] : m_EntityDataComponents)
                takenNames.insert(d.name);

            int n = 1;
            std::string candidate;
            do {
                candidate = base + " (" + std::to_string(n) + ")";
                ++n;
            } while (takenNames.count(candidate) > 0);

            data.name = candidate;
            AddEntityDataComponent(newEntity, data);
        }
        if (HasTransformComponent(source))
            AddTransformComponent(newEntity, *GetTransformComponent(source));
        if (HasMeshComponent(source))
            AddMeshComponent(newEntity, *GetMeshComponent(source));
        if (HasMaterialComponent(source))
            AddMaterialComponent(newEntity, *GetMaterialComponent(source));
        if (HasCameraComponent(source))
            AddCameraComponent(newEntity, *GetCameraComponent(source));
        if (HasScriptComponent(source))
            AddScriptComponent(newEntity, *GetScriptComponent(source));
        if (HasRigidbodyComponent(source))
            AddRigidbodyComponent(newEntity, *GetRigidbodyComponent(source));
        if (HasColliderComponent(source))
            AddColliderComponent(newEntity, *GetColliderComponent(source));
        if (HasPointLightComponent(source))
            AddPointLightComponent(newEntity, *GetPointLightComponent(source));

        // Inherit the source's parent so the duplicate appears as a sibling.
        EntityID parent = GetParent(source);
        if (parent != INVALID_ENTITY)
            SetParent(newEntity, parent);

        // Recursively duplicate the source's children under the new entity.
        if (duplicateChildren) {
            std::vector<EntityID> sourceChildren = GetChildren(source);  // copy
            for (EntityID child : sourceChildren) {
                EntityID newChild = DuplicateEntity(child, true);
                if (newChild != INVALID_ENTITY) {
                    // Recursive call attached newChild to the original child's parent (source).
                    // Reparent it under newEntity instead.
                    RemoveParent(newChild);
                    SetParent(newChild, newEntity);
                }
            }
        }

        return newEntity;
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
        newScene->m_RigidbodyComponents = m_RigidbodyComponents;
        newScene->m_ColliderComponents = m_ColliderComponents;
        newScene->m_PointLightComponents = m_PointLightComponents;
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


    // --- Rigidbody ---
    void Scene::AddRigidbodyComponent(EntityID entityID, const RigidbodyComponent& component)
    {
        m_RigidbodyComponents[entityID] = component;
    }

    RigidbodyComponent* Scene::GetRigidbodyComponent(EntityID entityID)
    {
        auto it = m_RigidbodyComponents.find(entityID);
        if (it != m_RigidbodyComponents.end())
            return &it->second;
        return nullptr;
    }

    bool Scene::HasRigidbodyComponent(EntityID entityID) const
    {
        return m_RigidbodyComponents.find(entityID) != m_RigidbodyComponents.end();
    }

    void Scene::RemoveRigidbodyComponent(EntityID entityID)
    {
        m_RigidbodyComponents.erase(entityID);
    }


    // --- Collider ---
    void Scene::AddColliderComponent(EntityID entityID, const ColliderComponent& component)
    {
        m_ColliderComponents[entityID] = component;
    }

    ColliderComponent* Scene::GetColliderComponent(EntityID entityID)
    {
        auto it = m_ColliderComponents.find(entityID);
        if (it != m_ColliderComponents.end())
            return &it->second;
        return nullptr;
    }

    bool Scene::HasColliderComponent(EntityID entityID) const
    {
        return m_ColliderComponents.find(entityID) != m_ColliderComponents.end();
    }

    void Scene::RemoveColliderComponent(EntityID entityID)
    {
        m_ColliderComponents.erase(entityID);
    }


    // --- Point Light ---
    void Scene::AddPointLightComponent(EntityID entityID, const PointLightComponent& component)
    {
        m_PointLightComponents[entityID] = component;
    }

    PointLightComponent* Scene::GetPointLightComponent(EntityID entityID)
    {
        auto it = m_PointLightComponents.find(entityID);
        if (it != m_PointLightComponents.end())
            return &it->second;
        return nullptr;
    }

    bool Scene::HasPointLightComponent(EntityID entityID) const
    {
        return m_PointLightComponents.find(entityID) != m_PointLightComponents.end();
    }

    void Scene::RemovePointLightComponent(EntityID entityID)
    {
        m_PointLightComponents.erase(entityID);
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