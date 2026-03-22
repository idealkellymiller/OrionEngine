#pragma once
#include "Entity.h"
#include "ComponentPool.h"
#include "Component.h"


class Scene {
public:
	Entity CreateEntity(const std::string& name = "SceneObject");
	void DestroyEntity(Entity entity);

    template<typename T>
    bool HasComponent(Entity entity) const;

    template<typename T>
    T& AddComponent(Entity entity, const T& component = T{});

    template<typename T>
    T* GetComponent(Entity entity);

    template<typename T>
    void RemoveComponent(Entity entity);

private:
    EntityID m_NextEntityID = 1;
    std::unordered_map<EntityID, EntityRecord> m_Entities;

    ComponentPool<TransformComponent> m_Transforms;
    ComponentPool<MeshComponent> m_Meshes;
    ComponentPool<CameraComponent> m_Cameras;
    ComponentPool<ScriptComponent> m_Scripts;
};