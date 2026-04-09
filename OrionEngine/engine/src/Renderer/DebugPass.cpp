#include "Renderer/DebugPass.h"
#include "Renderer/Gizmo.h"        // GizmoVertex struct
#include "Core/ProjectSettings.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <vector>
#include <cmath>
#include <iostream>

namespace Orion {

    // ---- helpers ----

    static void CreateDynamicLineMesh(unsigned int& vao, unsigned int& vbo)
    {
        if (vao == 0) {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
        }
    }

    static void UploadLineVertices(unsigned int vao, unsigned int vbo,
                                   const std::vector<GizmoVertex>& verts, int& outCount)
    {
        outCount = static_cast<int>(verts.size());

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(verts.size() * sizeof(GizmoVertex)),
                     verts.data(), GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(GizmoVertex), (void*)offsetof(GizmoVertex, Position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
            sizeof(GizmoVertex), (void*)offsetof(GizmoVertex, Color));

        glBindVertexArray(0);
    }

    static void DeleteLineMesh(unsigned int& vao, unsigned int& vbo)
    {
        if (vbo) { glDeleteBuffers(1, &vbo);       vbo = 0; }
        if (vao) { glDeleteVertexArrays(1, &vao);  vao = 0; }
    }

    // ---- Init / Shutdown ----

    void DebugPass::Init()
    {
        if (!m_Shader.CreateFromFiles(
                "../engine/shaders/Gizmo.vert",
                "../engine/shaders/Gizmo.frag"))
        {
            std::cout << "[DebugPass] Failed to load shader.\n";
        }

        BuildGrid();
    }

    void DebugPass::Shutdown()
    {
        DeleteLineMesh(m_ColliderVAO, m_ColliderVBO);
        DeleteLineMesh(m_GridVAO, m_GridVBO);
    }

    // ================================================================
    // Grid
    // ================================================================

    void DebugPass::BuildGrid(float halfExtent, float spacing)
    {
        std::vector<GizmoVertex> verts;

        // Subtle grey for minor lines, brighter for axes
        glm::vec3 minorColor(0.35f, 0.35f, 0.35f);
        glm::vec3 axisColorX(0.65f, 0.2f, 0.2f);   // red-ish for X axis
        glm::vec3 axisColorZ(0.2f, 0.2f, 0.65f);    // blue-ish for Z axis

        int lines = static_cast<int>(halfExtent / spacing);

        for (int i = -lines; i <= lines; ++i) {
            float pos = i * spacing;

            // Lines parallel to Z (varying X)
            glm::vec3 color = (i == 0) ? axisColorX : minorColor;
            verts.push_back({ { pos, 0.0f, -halfExtent }, color });
            verts.push_back({ { pos, 0.0f,  halfExtent }, color });

            // Lines parallel to X (varying Z)
            color = (i == 0) ? axisColorZ : minorColor;
            verts.push_back({ { -halfExtent, 0.0f, pos }, color });
            verts.push_back({ {  halfExtent, 0.0f, pos }, color });
        }

        CreateDynamicLineMesh(m_GridVAO, m_GridVBO);
        UploadLineVertices(m_GridVAO, m_GridVBO, verts, m_GridVertexCount);
    }

    void DebugPass::DrawGrid(const Camera& camera)
    {
        if (m_GridVAO == 0 || m_GridVertexCount == 0)
            return;

        glm::mat4 vp = camera.GetProjectionMatrix() * camera.GetViewMatrix();
        glm::mat4 model(1.0f); // grid is already in world space

        m_Shader.Bind();
        m_Shader.SetMat4("u_ViewProjection", vp);
        m_Shader.SetMat4("u_Model", model);
        m_Shader.SetVec3("u_ColorMultiplier", glm::vec3(1.0f));

        glBindVertexArray(m_GridVAO);

        // Render with depth test but write depth so objects occlude it
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDrawArrays(GL_LINES, 0, m_GridVertexCount);

        glBindVertexArray(0);
    }

    // ================================================================
    // Collider wireframe
    // ================================================================

    void DebugPass::UploadBoxWireframe(const glm::vec3& h)
    {
        // 12 edges of a box, each edge is 2 vertices = 24 vertices
        glm::vec3 color(0.0f, 1.0f, 0.0f); // green wireframe

        glm::vec3 corners[8] = {
            { -h.x, -h.y, -h.z }, {  h.x, -h.y, -h.z },
            {  h.x,  h.y, -h.z }, { -h.x,  h.y, -h.z },
            { -h.x, -h.y,  h.z }, {  h.x, -h.y,  h.z },
            {  h.x,  h.y,  h.z }, { -h.x,  h.y,  h.z },
        };

        int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},   // back face
            {4,5},{5,6},{6,7},{7,4},   // front face
            {0,4},{1,5},{2,6},{3,7},   // connecting edges
        };

        std::vector<GizmoVertex> verts;
        for (auto& e : edges) {
            verts.push_back({ corners[e[0]], color });
            verts.push_back({ corners[e[1]], color });
        }

        CreateDynamicLineMesh(m_ColliderVAO, m_ColliderVBO);
        UploadLineVertices(m_ColliderVAO, m_ColliderVBO, verts, m_ColliderVertexCount);
    }

    void DebugPass::UploadSphereWireframe(float radius, int segments)
    {
        glm::vec3 color(0.0f, 1.0f, 0.0f); // green wireframe
        std::vector<GizmoVertex> verts;

        // Three great circles (XY, XZ, YZ planes)
        auto addCircle = [&](int axisA, int axisB) {
            for (int i = 0; i < segments; ++i) {
                float a0 = glm::two_pi<float>() * float(i)     / float(segments);
                float a1 = glm::two_pi<float>() * float(i + 1) / float(segments);

                glm::vec3 p0(0.0f), p1(0.0f);
                p0[axisA] = cosf(a0) * radius;
                p0[axisB] = sinf(a0) * radius;
                p1[axisA] = cosf(a1) * radius;
                p1[axisB] = sinf(a1) * radius;

                verts.push_back({ p0, color });
                verts.push_back({ p1, color });
            }
        };

        addCircle(0, 1); // XY circle
        addCircle(0, 2); // XZ circle
        addCircle(1, 2); // YZ circle

        CreateDynamicLineMesh(m_ColliderVAO, m_ColliderVBO);
        UploadLineVertices(m_ColliderVAO, m_ColliderVBO, verts, m_ColliderVertexCount);
    }

    void DebugPass::DrawCollider(const Camera& camera, EntityID entity, Scene& scene)
    {
        ColliderComponent* col = scene.GetColliderComponent(entity);
        if (!col) return;

        TransformComponent* tc = scene.GetTransformComponent(entity);
        if (!tc) return;

        // Rebuild wireframe geometry for the current collider dimensions
        if (col->shape == ColliderShape::Box)
            UploadBoxWireframe(col->boxHalfExtents);
        else
            UploadSphereWireframe(col->sphereRadius);

        if (m_ColliderVAO == 0 || m_ColliderVertexCount == 0)
            return;

        // Model matrix: translate to entity position + collider offset
        // (Collider dimensions are in world space — no scale applied, matching PhysicsWorld)
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            tc->position + col->offset);

        // Apply entity rotation so the wireframe rotates with the object
        if (tc->rotation.x != 0.0f)
            model = glm::rotate(model, tc->rotation.x, glm::vec3(1, 0, 0));
        if (tc->rotation.y != 0.0f)
            model = glm::rotate(model, tc->rotation.y, glm::vec3(0, 1, 0));
        if (tc->rotation.z != 0.0f)
            model = glm::rotate(model, tc->rotation.z, glm::vec3(0, 0, 1));

        glm::mat4 vp = camera.GetProjectionMatrix() * camera.GetViewMatrix();

        m_Shader.Bind();
        m_Shader.SetMat4("u_ViewProjection", vp);
        m_Shader.SetMat4("u_Model", model);

        glBindVertexArray(m_ColliderVAO);

        glDisable(GL_CULL_FACE);

        // Pass 1: occluded (dim green) — visible through objects
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        m_Shader.SetVec3("u_ColorMultiplier", glm::vec3(0.25f));
        glDrawArrays(GL_LINES, 0, m_ColliderVertexCount);

        // Pass 2: visible (bright green)
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        m_Shader.SetVec3("u_ColorMultiplier", glm::vec3(1.0f));
        glDrawArrays(GL_LINES, 0, m_ColliderVertexCount);

        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }

}
