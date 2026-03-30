#include "Renderer/Gizmo.h"
#include "Renderer/Camera.h"

#include <vector>
#include <cstddef>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>


namespace Orion {


	void GizmoPass::Init()
	{
		// Load a very small unlit shader used only for the gizmo.
        if (!m_GizmoShader.CreateFromFiles(
            "../engine/engineAssets/shaders/Gizmo.vert",
            "../engine/engineAssets/shaders/Gizmo.frag"))
        {
            std::cout << "Failed to create gizmo shader\n";
        }

        std::vector<GizmoVertex> gizmoVertices =
        {
            // X axis - red
            { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
            { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },

            // Y axis - green
            { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
            { glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },

            // Z axis - blue
            { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
            { glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) }
        };

        // Create VAO/VBO for the gizmo line mesh
        glGenVertexArrays(1, &m_GizmoVAO);
        glGenBuffers(1, &m_GizmoVBO);

        glBindVertexArray(m_GizmoVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_GizmoVBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(gizmoVertices.size() * sizeof(GizmoVertex)),
            gizmoVertices.data(),
            GL_STATIC_DRAW
        );

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            sizeof(GizmoVertex),
            (void*)offsetof(GizmoVertex, Position)
        );

        // Color attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE,
            sizeof(GizmoVertex),
            (void*)offsetof(GizmoVertex, Color)
        );

        glBindVertexArray(0);
	}

    void GizmoPass::Shutdown()
    {
        if (m_GizmoVBO != 0)
        {
            glDeleteBuffers(1, &m_GizmoVBO);
            m_GizmoVBO = 0;
        }

        if (m_GizmoVAO != 0)
        {
            glDeleteVertexArrays(1, &m_GizmoVAO);
            m_GizmoVAO = 0;
        }
    }

    void GizmoPass::Execute(const Camera& camera, const GizmoData& gizmo)
    {
        // Force a known-good state for gizmo rendering.
        glDisable(GL_CULL_FACE); 
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        m_GizmoShader.Bind();

        // TODO: update this to show position is at the entity selected.
        glm::mat4 model = glm::translate(glm::mat4(1.0f), gizmo.position);
        model = glm::scale(model, glm::vec3(gizmo.axisLength));

        glm::mat4 viewProjection = camera.GetProjectionMatrix() * camera.GetViewMatrix();

        m_GizmoShader.SetMat4("u_ViewProjection", viewProjection);
        m_GizmoShader.SetMat4("u_Model", model);

        glBindVertexArray(m_GizmoVAO);
        glDrawArrays(GL_LINES, 0, 6);
        glBindVertexArray(0);

        // Restore depth writing for later passes.
        glDepthMask(GL_TRUE);
    }
}