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

    // Whether a rigidbody is dynamic (simulated), kinematic (script-driven), or static (immovable).
    enum class BodyType { Static, Kinematic, Dynamic };

    struct RigidbodyComponent {
        BodyType bodyType = BodyType::Dynamic;
        float mass = 1.0f;
        float linearDamping = 0.05f;
        float angularDamping = 0.05f;
        float gravityScale = 1.0f;     // 0 = no gravity, 1 = full gravity
        bool freezeRotationX = false;
        bool freezeRotationY = false;
        bool freezeRotationZ = false;
    };

    enum class ColliderShape { Box, Sphere };

    struct ColliderComponent {
        ColliderShape shape = ColliderShape::Box;
        glm::vec3 boxHalfExtents = { 0.5f, 0.5f, 0.5f };  // used when shape == Box
        float sphereRadius = 0.5f;                           // used when shape == Sphere
        bool isTrigger = false;                               // trigger = callbacks only, no physical response
        glm::vec3 offset = { 0.0f, 0.0f, 0.0f };            // local offset from entity transform
    };

    // Emits light from the entity's position in all directions.
    struct PointLightComponent {
        glm::vec3 color     = glm::vec3(1.0f, 1.0f, 1.0f);
        float intensity     = 1.0f;
        float constant      = 1.0f;
        float linear        = 0.09f;
        float quadratic     = 0.032f;
    };

    // Plays an audio clip at this entity's world position.
    // AudioEngine loads the clip at play-start and updates the 3D position each frame.
    struct AudioSourceComponent {
        std::string clipPath;           // relative to assets folder, e.g. "audio/boom.wav"
        float volume = 1.0f;
        float pitch  = 1.0f;
        bool  loop         = false;
        bool  playOnStart  = false;
        bool  spatial      = true;      // true = 3D positional; false = 2D non-positional
        float minDistance  = 1.0f;      // distance at which attenuation starts
        float maxDistance  = 20.0f;     // distance at which sound is inaudible
    };

    // Marks this entity as the audio listener (camera).
    // AudioEngine reads its TransformComponent each frame to update the listener pose.
    // If no listener exists in the scene, audio uses world-origin as the listener.
    struct AudioListenerComponent {
        // No configuration needed = existence of the component is sufficient.
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

        // Create a new entity that copies every component from `source`.
        // If duplicateChildren is true, the source's children are also duplicated
        // and attached under the new entity. The new entity inherits the source's
        // parent (so duplicates sit next to the original in the hierarchy).
        // Returns INVALID_ENTITY if source is not valid.
        EntityID DuplicateEntity(EntityID source, bool duplicateChildren = true);

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

        // Rigidbody
        void AddRigidbodyComponent(EntityID entityID, const RigidbodyComponent& component);
        RigidbodyComponent* GetRigidbodyComponent(EntityID entityID);
        bool HasRigidbodyComponent(EntityID entityID) const;
        void RemoveRigidbodyComponent(EntityID entityID);

        // Collider
        void AddColliderComponent(EntityID entityID, const ColliderComponent& component);
        ColliderComponent* GetColliderComponent(EntityID entityID);
        bool HasColliderComponent(EntityID entityID) const;
        void RemoveColliderComponent(EntityID entityID);

        // Point Light
        void AddPointLightComponent(EntityID entityID, const PointLightComponent& component);
        PointLightComponent* GetPointLightComponent(EntityID entityID);
        bool HasPointLightComponent(EntityID entityID) const;
        void RemovePointLightComponent(EntityID entityID);

        // Audio Source
        void AddAudioSourceComponent(EntityID entityID, const AudioSourceComponent& component);
        AudioSourceComponent* GetAudioSourceComponent(EntityID entityID);
        bool HasAudioSourceComponent(EntityID entityID) const;
        void RemoveAudioSourceComponent(EntityID entityID);

        // Audio Listener
        void AddAudioListenerComponent(EntityID entityID, const AudioListenerComponent& component);
        AudioListenerComponent* GetAudioListenerComponent(EntityID entityID);
        bool HasAudioListenerComponent(EntityID entityID) const;
        void RemoveAudioListenerComponent(EntityID entityID);

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
        std::unordered_map<EntityID, RigidbodyComponent> m_RigidbodyComponents;
        std::unordered_map<EntityID, ColliderComponent> m_ColliderComponents;
        std::unordered_map<EntityID, PointLightComponent> m_PointLightComponents;
        std::unordered_map<EntityID, AudioSourceComponent> m_AudioSourceComponents;
        std::unordered_map<EntityID, AudioListenerComponent> m_AudioListenerComponents;
        std::unordered_map<EntityID, RelationshipComponent> m_Relationships;

        // Empty children vector returned by GetChildren when entity has no relationships.
        static const std::vector<EntityID> s_EmptyChildren;
    };

}