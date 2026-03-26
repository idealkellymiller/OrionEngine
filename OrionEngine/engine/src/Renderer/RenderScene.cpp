#include "Renderer/RenderScene.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "Renderer/Material.h"
#include "Renderer/OBJLoader.h"
#include "Renderer/Renderer.h" // ============ POSSIBLE DEPENDENCY LOOP ================
#include "Renderer/EditorCamera.h"

#include "ECS/SceneManager.h"
#include "Assets/AssetManager.h"

#include <iostream>
#include <memory>



namespace Orion {

    // Read the current ECS scene and produce a temporary render-only snapshot for this frame
    void RenderScene::BuildRenderScene()
    {
        std::shared_ptr<Scene> scene = SceneManager::GetActiveScene();

        // Start fresh every frame;
        m_Renderables.clear();

        // Optional: avoid reallocations if scene size is stable.
        m_Renderables.reserve(scene->GetEntities().size());

        // Walk every entity in the scene
        for (const auto& entity : scene->GetEntities()) {

            // Check if the required ecs components exist
            if (!scene->HasTransformComponent(entity)) {
                std::cout << "Entity: " << entity << " does not have a transform component.\n";
                return;
            }
            if (!scene->HasMeshComponent(entity)) {
                std::cout << "Entity: " << entity << " does not have a mesh component.\n";
                return;
            }
            if (!scene->HasMaterialComponent(entity)) {
                std::cout << "Entity: " << entity << " does not have a material component.\n";
                return;
            }

            // Get the components
            TransformComponent* transformComp = scene->GetTransformComponent(entity);
            MeshComponent* meshComp = scene->GetMeshComponent(entity);
            MaterialComponent* materialComp = scene->GetMaterialComponent(entity);

            // Get the transform data
            glm::mat4 transform = glm::mat4(1.0f);
            // translate
            transform = glm::translate(transform, transformComp->position);
            // rotation
            if (transformComp->rotation.x != 0.0f)
                transform = glm::rotate(transform, transformComp->rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            if (transformComp->rotation.y != 0.0f)
                transform = glm::rotate(transform, transformComp->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            if (transformComp->rotation.z != 0.0f)
                transform = glm::rotate(transform, transformComp->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            // scale
            transform = glm::scale(transform, transformComp->scale);

            // Get the Mesh reference
            AssetID meshID = meshComp->mesh;
            std::shared_ptr<Mesh> mesh = AssetManager::GetMesh(meshID);

            // Get the Material reference
            AssetID materialID = materialComp->material;
            std::shared_ptr<Material> material = AssetManager::GetMaterial(materialID);


            Renderable renderable;
            renderable.entity = entity;
            renderable.worldTransform = transform;
            renderable.mesh = mesh;
            renderable.material = material;

            m_Renderables.push_back(renderable);
        }





        // Create and configure camera once.
        Camera camera;
        camera.SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));
        camera.SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
        camera.SetUp(glm::vec3(0.0f, 1.0f, 0.0f));
        Renderer::SetActiveCamera(camera);



        // Create a sun-like directional light.
        DirectionalLight sun;
        sun.Direction = glm::vec3(-0.5f, -1.0f, -0.2f);
        sun.Color = glm::vec3(1.0f, 0.95f, 0.85f);
        sun.Intensity = 1.2f;

        SetDirectionalLight(sun);
    }
}