// <Summary>
// No behavior inside of these, just data containers.
// <Summary>
#pragma once
#include <glm/glm.hpp>
#include <AssetTypes.h>


class Mesh;
class Material;


struct TransformComponent
{
    glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 RotationEuler{ 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };
};

struct MeshComponent
{
    AssetID meshAsset = INVALID_ASSET;
    AssetID materialOverride = INVALID_ASSET;
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