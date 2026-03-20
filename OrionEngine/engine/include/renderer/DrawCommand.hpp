#pragma once

#include <glm/glm.hpp>

class Mesh;
class Material;

struct DrawCommand
{
    Mesh* MeshPtr = nullptr;
    Material* MaterialPtr = nullptr;
    glm::mat4 ModelMatrix = glm::mat4(1.0f);
    float CameraDistance = 0.0f;
    unsigned int SortKey = 0;
    bool CastsShadows = true;
};