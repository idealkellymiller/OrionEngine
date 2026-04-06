#pragma once
#include <glm/glm.hpp>

#include "Renderer/Camera.h"
#include "Renderer/Shader.h"


namespace Orion {

    enum class GizmoMode
    {
        Translate = 0,
        Rotate,
        Scale
    };

    // Which axis (if any) is being interacted with.
    enum class GizmoAxis
    {
        None = 0,
        X,
        Y,
        Z
    };

    struct GizmoData
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::mat4 orientation = glm::mat4(1.0f); // object's rotation matrix
        float axisLength = 1.0f;
        GizmoMode mode = GizmoMode::Translate;

        // Get the world-space direction for a given local axis.
        glm::vec3 GetAxisDir(GizmoAxis axis) const;
    };

    struct GizmoVertex
    {
        glm::vec3 Position;
        glm::vec3 Color;
    };

    // Gizmo interaction: hit-testing and ray helpers.
    class ORION_API GizmoInteraction {
    public:
        static GizmoAxis HitTest(
            const Camera& camera,
            const GizmoData& gizmo,
            float mouseViewportX,
            float mouseViewportY,
            float viewportWidth,
            float viewportHeight
        );

        // Project a world-space point onto the screen (viewport pixel coords).
        static glm::vec2 WorldToViewport(
            const glm::vec3& worldPos,
            const Camera& camera,
            float viewportWidth,
            float viewportHeight
        );

        // Build a world-space ray from a viewport pixel position.
        static void ViewportToRay(
            float mouseViewportX,
            float mouseViewportY,
            float viewportWidth,
            float viewportHeight,
            const Camera& camera,
            glm::vec3& outOrigin,
            glm::vec3& outDir
        );

        // Intersect a ray with a plane. Returns false if nearly parallel.
        static bool RayPlaneIntersect(
            const glm::vec3& rayOrigin,
            const glm::vec3& rayDir,
            const glm::vec3& planePoint,
            const glm::vec3& planeNormal,
            float& outT
        );
    };


    class ORION_API GizmoPass {
    public:
        void Init();
        void Shutdown();

        void Execute(const Camera& camera, const GizmoData& gizmo);

    private:
        void BuildTranslateGeometry();
        void BuildRotateGeometry();
        void BuildScaleGeometry();

        unsigned int m_TranslateVAO = 0;
        unsigned int m_TranslateVBO = 0;
        int m_TranslateVertexCount = 0;

        unsigned int m_RotateVAO = 0;
        unsigned int m_RotateVBO = 0;
        int m_RotateVertexCount = 0;

        unsigned int m_ScaleVAO = 0;
        unsigned int m_ScaleVBO = 0;
        int m_ScaleVertexCount = 0;

        Shader m_GizmoShader;
    };
}
