#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>


class Scene;
class Mesh;
class Material;

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

struct TransformComponent
{
    glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 RotationEuler{ 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };
};

struct MeshComponent
{
    Mesh& Mesh;
    Material& Material;
    bool Visible = true;
};

struct CameraComponent
{
    float Fov = 60.0f;
    float NearClip = 0.1f;
    float FarClip = 1000.0f;
    bool Primary = false;
};

struct ScriptComponent
{
    std::string ScriptClassName;
    bool Enabled = true;
};