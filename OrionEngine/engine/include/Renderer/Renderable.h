#pragma once
#include <cstdint>

#include "Renderer/Handles.h"


class Renderable {
public:
    uint32_t entityID;
    MeshHandle mesh;
    MaterialHandle material;
    // Mat4 modelMatrix;
    // AABB boundingBox;
    uint32_t layerMask = 0xFFFFFFFF; // Default to all layers
};