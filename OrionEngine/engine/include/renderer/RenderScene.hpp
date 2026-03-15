#pragma once

#include <vector>
#include "Renderable.hpp"
#include "DirectionalLight.hpp"

class Camera;

class RenderScene
{
public:
    void SetActiveCamera(Camera* camera)
    {
        m_ActiveCamera = camera;
    }

    Camera* GetActiveCamera() const
    {
        return m_ActiveCamera;
    }

    void AddRenderable(const Renderable& renderable)
    {
        m_Renderables.push_back(renderable);
    }

    const std::vector<Renderable>& GetRenderables() const
    {
        return m_Renderables;
    }

    void Clear()
    {
        m_Renderables.clear();
        m_ActiveCamera = nullptr;
        m_HasDirectionalLight = false;
    }

    // Set the scene's main directional light.
    void SetDirectionalLight(const DirectionalLight& light)
    {
        m_DirectionalLight = light;
        m_HasDirectionalLight = true;
    }

    bool HasDirectionalLight() const
    {
        return m_HasDirectionalLight;
    }

    const DirectionalLight& GetDirectionalLight() const
    {
        return m_DirectionalLight;
    }

private:
    std::vector<Renderable> m_Renderables;
    Camera* m_ActiveCamera = nullptr;

    DirectionalLight m_DirectionalLight;
    bool m_HasDirectionalLight = false;
};