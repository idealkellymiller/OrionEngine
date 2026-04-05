// Owns entities/components

#pragma once
#include "EngineCore.h"
#include <cstdint>
#include <vector>
#include <unordered_map>
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

        // Material
        void AddMaterialComponent(EntityID entityID, const MaterialComponent& component);
        MaterialComponent* GetMaterialComponent(EntityID entityID);
        bool HasMaterialComponent(EntityID entityID) const;

    private:
        EntityID m_NextEntityID = 1;

        std::vector<EntityID> m_Entities;

        std::unordered_map<EntityID, EntityDataComponent> m_EntityDataComponents;
        std::unordered_map<EntityID, TransformComponent> m_TransformComponents;
        std::unordered_map<EntityID, MeshComponent> m_MeshComponents;
        std::unordered_map<EntityID, MaterialComponent> m_MaterialComponents;
    };

}