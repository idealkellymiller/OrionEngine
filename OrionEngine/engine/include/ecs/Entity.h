#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>


class Scene;

using EntityID = uint32_t;
static constexpr EntityID INVALID_ENTITY = 0;

class Entity {
public:
	Entity() = default;
	Entity(EntityID id, Scene* scene) : m_ID(id), m_Scene(scene) {}

	EntityID GetID() const { return m_ID; }
	bool IsValid() const { return m_ID != INVALID_ENTITY && m_Scene != nullptr; }

private:
	EntityID m_ID = INVALID_ENTITY;
	Scene* m_Scene = nullptr;
};


struct EntityRecord
{
	std::string Name = "SceneObject";
	EntityID Parent = INVALID_ENTITY;
	std::vector<EntityID> Children;
	bool Enabled = true;
};
