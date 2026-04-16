// <Summary>
// RenderScene is jsut a submission packet for the renderer.
// <Summary>
#pragma once
#include "EngineCore.h"

#include <vector>
#include <memory>
#include "DirectionalLight.h"
#include "ECS/Scene.h"




namespace Orion {

    class Mesh;
    class Material;
    class Camera;
    class AssetManager;


    struct ORION_API Renderable {
        EntityID entity = INVALID_ENTITY;

        glm::mat4 worldTransform = glm::mat4(1.0f);	            // model matrix

        std::shared_ptr<Mesh> mesh = nullptr;					// Geometry to draw
        std::shared_ptr<Material> material = nullptr;			// Appearance to use
    };


    class ORION_API RenderScene {
    public:
        void Clear() { m_Renderables.clear(); }
        void BuildRenderScene();//const Scene& scene, AssetManager& assetManager);

        //void SetActiveCamera(Camera* camera) { m_ActiveCamera = camera; }
        //Camera* GetActiveCamera() const { return m_ActiveCamera; }

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

        void AddPointLight(const PointLight& light) {
            if ((int)m_PointLights.size() < MAX_POINT_LIGHTS)
                m_PointLights.push_back(light);
        }

        const std::vector<PointLight>& GetPointLights() const { return m_PointLights; }
        int GetPointLightCount() const { return (int)m_PointLights.size(); }

    private:
        std::vector<Renderable> m_Renderables;
        Camera* m_ActiveCamera = nullptr;

        DirectionalLight m_DirectionalLight;
        bool m_HasDirectionalLight = false;

        std::vector<PointLight> m_PointLights;
    };
}