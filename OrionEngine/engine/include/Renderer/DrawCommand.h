#pragma once
#include "EngineCore.h"
#include <glm/glm.hpp>
#include <memory>
#include "ECS/Scene.h"


namespace Orion {

    class Mesh;
    class Material;

    struct ORION_API DrawCommand
    {
        std::shared_ptr<Mesh> MeshPtr = nullptr;
        std::shared_ptr<Material> MaterialPtr = nullptr;
        glm::mat4 ModelMatrix = glm::mat4(1.0f);
        float CameraDistance = 0.0f;
        unsigned int SortKey = 0;
        bool CastsShadows = true;

        EntityID Entity = INVALID_ENTITY;
    };
}