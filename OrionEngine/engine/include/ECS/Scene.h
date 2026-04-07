// Owns entities/components

#pragma once
#include "EngineCore.h"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

#include "Assets/AssetTypes.h"


namespace Orion {

    using EntityID = uint32_t;
    static constexpr EntityID INVALID_ENTITY = 0;

    // metadata for this entity
    struct EntityDataComponent {
        std::string name = "New Entity";
        bool enabled = true;
    };

    struct TransformComponent {
        glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    };

    struct MeshComponent {
        // References to the mesh and material assets loaded in the file directory.
        AssetID mesh = INVALID_ASSET_ID;
    };

    struct MaterialComponent {
        AssetID material = INVALID_ASSET_ID;
    };

    struct CameraComponent {
        float fovDegrees = 60.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        bool isActive = true;   // If multiple cameras exist, only the active one is used
    };

    struct RelationshipComponent {
        EntityID parent = INVALID_ENTITY;
        std::vector<EntityID> children;
    };

    // Points to a Lua script file that defines OnStart() and OnUpdate(dt).
    // The ScriptEngine (Apollo) loads and runs these during play mode.
    struct ScriptComponent {
        std::string scriptPath;  // Relative to assets folder, e.g. "scripts/rotate.lua"
    };







    class ORION_API Scene {
    public:
        Scene();
        ~Scene();

        // Entity Management
        EntityID CreateEntity();
        EntityID CreateEntityWithID(EntityID entityID);
        void DestroyEntity(EntityID entityID);
        bool IsValidEntity(EntityID entityID) const;

        const std::vector<EntityID>& GetEntities() const { return m_Entities; }
        const EntityID GetNextEntityID() { return m_NextEntityID; }

        void Clear();

        // Deep-copy this scene into a new Scene (for play-mode snapshot/restore)
        std::shared_ptr<Scene> Copy() const;

        // Entity data
        void AddEntityDataComponent(EntityID entityID, const EntityDataComponent& component);
        EntityDataComponent* GetEntityDataComponent(EntityID entityID);
        bool HasEntityDataComponent(EntityID entityID) const;

        // Transform
        void AddTransformComponent(EntityID entityID, const TransformComponent& component);
        TransformComponent* GetTransformComponent(EntityID entityID);
        bool HasTransformComponent(EntityID entityID) const;

        // Mesh
        void AddMeshComponent(EntityID entityID, const MeshComponent& component);
        MeshComponent* GetMeshComponent(EntityID entityID);
        bool HasMeshComponent(EntityID entityID) const;
        void RemoveMeshComponent(EntityID entityID);

        // Material
        void AddMaterialComponent(EntityID entityID, const MaterialComponent& component);
        MaterialComponent* GetMaterialComponent(EntityID entityID);
        bool HasMaterialComponent(EntityID entityID) const;
        void RemoveMaterialComponent(EntityID entityID);

        // Camera
        void AddCameraComponent(EntityID entityID, const CameraComponent& component);
        CameraComponent* GetCameraComponent(EntityID entityID);
        bool HasCameraComponent(EntityID entityID) const;
        void RemoveCameraComponent(EntityID entityID);

        // Script
        void AddScriptComponent(EntityID entityID, const ScriptComponent& component);
        ScriptComponent* GetScriptComponent(EntityID entityID);
        bool HasScriptComponent(EntityID entityID) const;
        void RemoveScriptComponent(EntityID entityID);

        // Relationships (parent-child hierarchy)
        void SetParent(EntityID child, EntityID parent);
        void RemoveParent(EntityID child);
        EntityID GetParent(EntityID entityID) const;
        const std::vector<EntityID>& GetChildren(EntityID entityID) const;
        bool HasChildren(EntityID entityID) const;
        bool HasParent(EntityID entityID) const;

        // Returns the list of root entities (entities with no parent), in order.
        std::vector<EntityID> GetRootEntities() const;

        // Compute world transform by walking up the parent chain.
        // Returns parentWorldTransform * localTransform.
        glm::mat4 GetWorldTransform(EntityID entityID) const;

    private:
        // Helper: build a local transform matrix from a TransformComponent.
        static glm::mat4 BuildLocalTransform(const TransformComponent& tc);

        EntityID m_NextEntityID = 1;

        std::vector<EntityID> m_Entities;

        std::unordered_map<EntityID, EntityDataComponent> m_EntityDataComponents;
        std::unordered_map<EntityID, TransformComponent> m_TransformComponents;
        std::unordered_map<EntityID, MeshComponent> m_MeshComponents;
        std::unordered_map<EntityID, MaterialComponent> m_MaterialComponents;
        std::unordered_map<EntityID, CameraComponent> m_CameraComponents;
        std::unordered_map<EntityID, ScriptComponent> m_ScriptComponents;
        std::unordered_map<EntityID, RelationshipComponent> m_Relationships;

        // Empty children vector returned by GetChildren when entity has no relationships.
        static const std::vector<EntityID> s_EmptyChildren;
    };

}