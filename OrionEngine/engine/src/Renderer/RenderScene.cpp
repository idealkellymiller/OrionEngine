#include "Renderer/RenderScene.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "Renderer/Material.h"
#include "Renderer/OBJLoader.h"
#include "Renderer/Renderer.h" // ============ POSSIBLE DEPENDENCY LOOP ================

#include "ECS/SceneManager.h"
#include "Assets/AssetManager.h"
#include "Core/ProjectSettings.h"

#include <iostream>
#include <memory>



namespace Orion {

    // Read the current ECS scene and produce a temporary render-only snapshot for this frame
    void RenderScene::BuildRenderScene()
    {
        std::shared_ptr<Scene> scene = SceneManager::GetActiveScene();

        // Start fresh every frame;
        m_Renderables.clear();
        m_PointLights.clear();

        // Optional: avoid reallocations if scene size is stable.
        m_Renderables.reserve(scene->GetEntities().size());

        // Walk every entity in the scene
        for (const auto& entity : scene->GetEntities()) {

            // Skip entities that don't have the components needed for rendering.
            // Not every entity is renderable (e.g. camera-only entities).
            if (!scene->HasTransformComponent(entity) ||
                !scene->HasMeshComponent(entity) ||
                !scene->HasMaterialComponent(entity)) {
                continue;
            }

            // Get the components
            MeshComponent* meshComp = scene->GetMeshComponent(entity);
            MaterialComponent* materialComp = scene->GetMaterialComponent(entity);

            // Compute world transform (walks up parent chain if parented)
            glm::mat4 transform = scene->GetWorldTransform(entity);

            // Get the Mesh reference
            AssetID meshID = meshComp->mesh;
            std::shared_ptr<Mesh> mesh = AssetManager::GetMesh(meshID);

            // Get the Material reference
            AssetID materialID = materialComp->material;
            std::shared_ptr<Material> material = AssetManager::GetMaterial(materialID);

            // Skip entities with missing assets to avoid null dereferences in the renderer
            if (!mesh || !material) {
                // std::cout << "Warning: Entity " << entity << " has invalid mesh or material asset. Skipping.\n";
                continue;
            }

            Renderable renderable;
            renderable.entity = entity;
            renderable.worldTransform = transform;
            renderable.mesh = mesh;
            renderable.material = material;

            m_Renderables.push_back(renderable);
        }





        // Collect point lights from entities with PointLightComponent
        for (const auto& entity : scene->GetEntities()) {
            if (!scene->HasPointLightComponent(entity) || !scene->HasTransformComponent(entity))
                continue;

            PointLightComponent* plc = scene->GetPointLightComponent(entity);
            TransformComponent* tc = scene->GetTransformComponent(entity);

            // Use world position (accounts for parenting)
            glm::mat4 worldMat = scene->GetWorldTransform(entity);
            glm::vec3 worldPos = glm::vec3(worldMat[3]);

            PointLight light;
            light.Position  = worldPos;
            light.Color     = plc->color;
            light.Intensity = plc->intensity;
            light.Constant  = plc->constant;
            light.Linear    = plc->linear;
            light.Quadratic = plc->quadratic;

            AddPointLight(light);
        }

        // Camera is managed by EditorCamera and written into
        // Renderer::GetActiveCamera() each frame — no override here.

        // Directional light — driven by ProjectSettings so the user can tweak it.
        ProjectSettings& settings = ProjectSettings::Get();
        DirectionalLight sun;
        sun.Direction = settings.sunDirection;
        sun.Color     = settings.sunColor;
        sun.Intensity = settings.sunIntensity;

        SetDirectionalLight(sun);
    }
}