// <Summary>
// RenderScene is jsut a submission packet for the renderer.
// <Summary>
#pragma once

#include <vector>
#include "DirectionalLight.hpp"
#include "Entity.h"


class Mesh;
class Material;
class Camera;
class AssetManager;


struct Renderable {
    EntityID Entity = INVALID_ENTITY;
    Mesh* MeshPtr = nullptr;					// Geometry to draw
    Material* MaterialPtr = nullptr;			// Appearance to use
    glm::mat4 ModelMatrix = glm::mat4(1.0f);	// Final world transform

    // Optional future-facing fields:
    // Layer or render group lets you filter objects later
    // without touching the ECS.
    unsigned int RenderLayer = 0;

    // usefule for skipping hidden editor/debug objects.
    bool Visible = true;

    bool IsValid() const {
        // A renerable is drawable only of it has both a mesh and a material
        return Visible && MeshPtr != nullptr && MaterialPtr != nullptr;
    }
};


class RenderScene {
public:  
    void Clear();
    void BuildRenderScene(const Scene& scene, AssetManager& assetManager);

    void SetActiveCamera(Camera* camera) { m_ActiveCamera = camera; }
    Camera* GetActiveCamera() const { return m_ActiveCamera; }

    void AddRenderable(const Renderable& renderable) { m_Renderables.push_back(renderable); }
    const std::vector<Renderable>& GetRenderables() const { return m_Renderables; }

    // Set the scene's main directional light.
    void SetDirectionalLight(const DirectionalLight& light)
    {
        m_DirectionalLight = light;
        m_HasDirectionalLight = true;
    }

    bool HasDirectionalLight() const { return m_HasDirectionalLight; }

    const DirectionalLight& GetDirectionalLight() const { return m_DirectionalLight; }

private:
    std::vector<Renderable> m_Renderables;
    Camera* m_ActiveCamera = nullptr;

    DirectionalLight m_DirectionalLight;
    bool m_HasDirectionalLight = false;
};