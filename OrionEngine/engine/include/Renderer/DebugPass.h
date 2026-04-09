#pragma once
#include "EngineCore.h"
#include "Renderer/Shader.h"
#include "Renderer/Camera.h"
#include "ECS/Scene.h"

#include <glm/glm.hpp>

namespace Orion {

    // Lightweight debug renderer that draws:
    //   - Wireframe collider for the selected entity
    //   - World-space XZ grid (toggleable via ProjectSettings)
    //
    // Uses the same GizmoVertex layout (position + color) and Gizmo shader
    // so we don't need extra shader files.

    class ORION_API DebugPass {
    public:
        void Init();
        void Shutdown();

        // Rebuild the grid mesh (call once or when grid settings change).
        void BuildGrid(float halfExtent = 50.0f, float spacing = 1.0f);

        // Draw the collider wireframe for one entity.
        void DrawCollider(const Camera& camera, EntityID entity, Scene& scene);

        // Draw the XZ world grid.
        void DrawGrid(const Camera& camera);

    private:
        // Build wireframe geometry for box / sphere into a dynamic VBO.
        void UploadBoxWireframe(const glm::vec3& halfExtents);
        void UploadSphereWireframe(float radius, int segments = 32);

        Shader m_Shader;   // reuses Gizmo.vert / Gizmo.frag

        // Collider wireframe (rebuilt per-frame for selected entity)
        unsigned int m_ColliderVAO = 0;
        unsigned int m_ColliderVBO = 0;
        int          m_ColliderVertexCount = 0;

        // Grid (built once)
        unsigned int m_GridVAO = 0;
        unsigned int m_GridVBO = 0;
        int          m_GridVertexCount = 0;
    };

}
