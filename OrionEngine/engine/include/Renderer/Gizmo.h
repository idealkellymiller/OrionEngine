#pragma once
#include <glm/glm.hpp>

#include "Renderer/Camera.h"
#include "Renderer/Shader.h"


namespace Orion {

    struct GizmoData
    {
        glm::vec3 position = glm::vec3(0.0f);
        float axisLength = 1.0f;
    };

    struct GizmoVertex
    {
        glm::vec3 Position;
        glm::vec3 Color;
    };


    class ORION_API GizmoPass {
    public:
        void Init();
        void Shutdown();

        void Execute(const Camera& camera, const GizmoData& gizmo);

    private:
        unsigned int m_GizmoVAO = 0;
        unsigned int m_GizmoVBO = 0;

        Shader m_GizmoShader;
    };
}