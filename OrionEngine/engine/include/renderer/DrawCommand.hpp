#pragma once
#include <glm/glm.hpp>
#include <memory>

class Mesh;
class Material;

struct DrawCommand
{
    std::shared_ptr<Mesh> MeshPtr = nullptr;
    std::shared_ptr<Material> MaterialPtr = nullptr;
    glm::mat4 ModelMatrix = glm::mat4(1.0f);
    float CameraDistance = 0.0f;
    unsigned int SortKey = 0;
    bool CastsShadows = true;
};