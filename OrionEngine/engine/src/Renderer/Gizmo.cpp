#include "Renderer/Gizmo.h"
#include "Renderer/Camera.h"

#include <vector>
#include <cstddef>
#include <cmath>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>


namespace Orion {

    // ---- GizmoData ----

    glm::vec3 GizmoData::GetAxisDir(GizmoAxis axis) const
    {
        // Extract the oriented local axis from the orientation matrix.
        switch (axis)
        {
        case GizmoAxis::X: return glm::normalize(glm::vec3(orientation[0]));
        case GizmoAxis::Y: return glm::normalize(glm::vec3(orientation[1]));
        case GizmoAxis::Z: return glm::normalize(glm::vec3(orientation[2]));
        default:           return glm::vec3(0.0f);
        }
    }

    // ---- GPU helpers ----

    static void UploadGizmoMesh(
        const std::vector<GizmoVertex>& vertices,
        unsigned int& vao, unsigned int& vbo, int& vertexCount)
    {
        vertexCount = static_cast<int>(vertices.size());

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(GizmoVertex)),
            vertices.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(GizmoVertex), (void*)offsetof(GizmoVertex, Position));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
            sizeof(GizmoVertex), (void*)offsetof(GizmoVertex, Color));

        glBindVertexArray(0);
    }

    static void DeleteGizmoMesh(unsigned int& vao, unsigned int& vbo)
    {
        if (vbo != 0) { glDeleteBuffers(1, &vbo);       vbo = 0; }
        if (vao != 0) { glDeleteVertexArrays(1, &vao);  vao = 0; }
    }


    // ---- Translate geometry ----

    void GizmoPass::BuildTranslateGeometry()
    {
        const float arrow = 0.12f;
        std::vector<GizmoVertex> verts;

        auto addAxis = [&](glm::vec3 dir, glm::vec3 perp1, glm::vec3 perp2, glm::vec3 color)
        {
            glm::vec3 origin(0.0f);
            glm::vec3 tip = dir;
            glm::vec3 back = dir * (1.0f - arrow);

            verts.push_back({ origin, color });
            verts.push_back({ tip,    color });

            verts.push_back({ tip, color });
            verts.push_back({ back + perp1 * arrow, color });
            verts.push_back({ tip, color });
            verts.push_back({ back - perp1 * arrow, color });
            verts.push_back({ tip, color });
            verts.push_back({ back + perp2 * arrow, color });
            verts.push_back({ tip, color });
            verts.push_back({ back - perp2 * arrow, color });
        };

        addAxis({ 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 });
        addAxis({ 0, 1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 });
        addAxis({ 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 });

        UploadGizmoMesh(verts, m_TranslateVAO, m_TranslateVBO, m_TranslateVertexCount);
    }


    // ---- Rotate geometry ----

    void GizmoPass::BuildRotateGeometry()
    {
        const int segments = 64;
        std::vector<GizmoVertex> verts;

        auto addCircle = [&](int axis, glm::vec3 color)
        {
            for (int i = 0; i < segments; ++i)
            {
                float a0 = glm::two_pi<float>() * float(i)     / float(segments);
                float a1 = glm::two_pi<float>() * float(i + 1) / float(segments);

                glm::vec3 p0(0.0f), p1(0.0f);

                if (axis == 0) {
                    p0 = { 0.0f, cosf(a0), sinf(a0) };
                    p1 = { 0.0f, cosf(a1), sinf(a1) };
                } else if (axis == 1) {
                    p0 = { cosf(a0), 0.0f, sinf(a0) };
                    p1 = { cosf(a1), 0.0f, sinf(a1) };
                } else {
                    p0 = { cosf(a0), sinf(a0), 0.0f };
                    p1 = { cosf(a1), sinf(a1), 0.0f };
                }

                verts.push_back({ p0, color });
                verts.push_back({ p1, color });
            }
        };

        addCircle(0, { 1, 0, 0 });
        addCircle(1, { 0, 1, 0 });
        addCircle(2, { 0, 0, 1 });

        UploadGizmoMesh(verts, m_RotateVAO, m_RotateVBO, m_RotateVertexCount);
    }


    // ---- Scale geometry ----

    void GizmoPass::BuildScaleGeometry()
    {
        const float boxHalf = 0.06f;
        std::vector<GizmoVertex> verts;

        auto addAxis = [&](glm::vec3 dir, glm::vec3 perp1, glm::vec3 perp2, glm::vec3 color)
        {
            glm::vec3 origin(0.0f);
            glm::vec3 tip = dir;

            verts.push_back({ origin, color });
            verts.push_back({ tip,    color });

            glm::vec3 c = tip;
            glm::vec3 d1 = perp1 * boxHalf;
            glm::vec3 d2 = perp2 * boxHalf;
            glm::vec3 d3 = dir   * boxHalf;

            glm::vec3 corners[8] = {
                c-d1-d2-d3, c+d1-d2-d3, c+d1+d2-d3, c-d1+d2-d3,
                c-d1-d2+d3, c+d1-d2+d3, c+d1+d2+d3, c-d1+d2+d3,
            };

            int edges[12][2] = {
                {0,1},{1,2},{2,3},{3,0},
                {4,5},{5,6},{6,7},{7,4},
                {0,4},{1,5},{2,6},{3,7},
            };

            for (auto& e : edges)
            {
                verts.push_back({ corners[e[0]], color });
                verts.push_back({ corners[e[1]], color });
            }
        };

        addAxis({ 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 });
        addAxis({ 0, 1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 });
        addAxis({ 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 });

        UploadGizmoMesh(verts, m_ScaleVAO, m_ScaleVBO, m_ScaleVertexCount);
    }


    // ---- Init / Shutdown ----

    void GizmoPass::Init()
    {
        if (!m_GizmoShader.CreateFromFiles(
            "../engine/shaders/Gizmo.vert",
            "../engine/shaders/Gizmo.frag"))
        {
            std::cout << "Failed to create gizmo shader\n";
        }

        BuildTranslateGeometry();
        BuildRotateGeometry();
        BuildScaleGeometry();
    }

    void GizmoPass::Shutdown()
    {
        DeleteGizmoMesh(m_TranslateVAO, m_TranslateVBO);
        DeleteGizmoMesh(m_RotateVAO,    m_RotateVBO);
        DeleteGizmoMesh(m_ScaleVAO,     m_ScaleVBO);
    }


    // ---- Execute ----

    void GizmoPass::Execute(const Camera& camera, const GizmoData& gizmo)
    {
        unsigned int vao = 0;
        int vertCount = 0;

        switch (gizmo.mode)
        {
        case GizmoMode::Translate:  vao = m_TranslateVAO; vertCount = m_TranslateVertexCount; break;
        case GizmoMode::Rotate:     vao = m_RotateVAO;    vertCount = m_RotateVertexCount;    break;
        case GizmoMode::Scale:      vao = m_ScaleVAO;     vertCount = m_ScaleVertexCount;     break;
        }

        if (vao == 0 || vertCount == 0)
            return;

        // Model matrix: translate → orient → scale by axis length.
        // Geometry is built in local space (unit axes), orientation rotates it,
        // axisLength scales it up, and translation places it in the world.
        glm::mat4 model = glm::translate(glm::mat4(1.0f), gizmo.position);
        model = model * gizmo.orientation;
        model = glm::scale(model, glm::vec3(gizmo.axisLength));

        glm::mat4 viewProjection = camera.GetProjectionMatrix() * camera.GetViewMatrix();

        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        m_GizmoShader.Bind();
        m_GizmoShader.SetMat4("u_ViewProjection", viewProjection);
        m_GizmoShader.SetMat4("u_Model", model);

        glBindVertexArray(vao);

        // Pass 1: occluded (dim)
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        m_GizmoShader.SetVec3("u_ColorMultiplier", glm::vec3(0.35f));
        glDrawArrays(GL_LINES, 0, vertCount);

        // Pass 2: visible (bright)
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        m_GizmoShader.SetVec3("u_ColorMultiplier", glm::vec3(1.0f));
        glDrawArrays(GL_LINES, 0, vertCount);

        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }


    // =========================================================================
    // GizmoInteraction
    // =========================================================================

    glm::vec2 GizmoInteraction::WorldToViewport(
        const glm::vec3& worldPos,
        const Camera& camera,
        float viewportWidth,
        float viewportHeight)
    {
        glm::mat4 vp = camera.GetProjectionMatrix() * camera.GetViewMatrix();
        glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);

        if (clip.w <= 0.0f)
            return glm::vec2(-1e6f);

        glm::vec3 ndc = glm::vec3(clip) / clip.w;

        float sx = (ndc.x * 0.5f + 0.5f) * viewportWidth;
        float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportHeight;
        return glm::vec2(sx, sy);
    }

    void GizmoInteraction::ViewportToRay(
        float mouseViewportX,
        float mouseViewportY,
        float viewportWidth,
        float viewportHeight,
        const Camera& camera,
        glm::vec3& outOrigin,
        glm::vec3& outDir)
    {
        // Viewport pixel → NDC
        float ndcX = (mouseViewportX / viewportWidth)  * 2.0f - 1.0f;
        float ndcY = 1.0f - (mouseViewportY / viewportHeight) * 2.0f; // flip Y

        glm::mat4 invVP = glm::inverse(camera.GetProjectionMatrix() * camera.GetViewMatrix());

        glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farPoint  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);

        nearPoint /= nearPoint.w;
        farPoint  /= farPoint.w;

        outOrigin = glm::vec3(nearPoint);
        outDir    = glm::normalize(glm::vec3(farPoint - nearPoint));
    }

    bool GizmoInteraction::RayPlaneIntersect(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDir,
        const glm::vec3& planePoint,
        const glm::vec3& planeNormal,
        float& outT)
    {
        float denom = glm::dot(planeNormal, rayDir);
        if (fabsf(denom) < 1e-6f)
            return false; // Nearly parallel

        outT = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
        return true;
    }

    // 2D distance from point to line segment.
    static float DistanceToSegment(glm::vec2 p, glm::vec2 a, glm::vec2 b)
    {
        glm::vec2 ab = b - a;
        float lenSq = glm::dot(ab, ab);
        if (lenSq < 1e-8f)
            return glm::length(p - a);

        float t = glm::clamp(glm::dot(p - a, ab) / lenSq, 0.0f, 1.0f);
        glm::vec2 closest = a + ab * t;
        return glm::length(p - closest);
    }

    // Hit-test against oriented axis lines (translate/scale).
    static GizmoAxis HitTestLines(
        const glm::vec2& mouse,
        const Camera& camera,
        const GizmoData& gizmo,
        float viewportWidth, float viewportHeight,
        float threshold)
    {
        glm::vec2 origin = GizmoInteraction::WorldToViewport(
            gizmo.position, camera, viewportWidth, viewportHeight);

        GizmoAxis candidates[] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };

        GizmoAxis best = GizmoAxis::None;
        float bestDist = threshold;

        for (auto ax : candidates)
        {
            glm::vec3 dir = gizmo.GetAxisDir(ax);
            glm::vec3 tipWorld = gizmo.position + dir * gizmo.axisLength;
            glm::vec2 tip = GizmoInteraction::WorldToViewport(
                tipWorld, camera, viewportWidth, viewportHeight);

            float dist = DistanceToSegment(mouse, origin, tip);
            if (dist < bestDist)
            {
                bestDist = dist;
                best = ax;
            }
        }
        return best;
    }

    // Hit-test against oriented circles (rotation).
    static GizmoAxis HitTestCircles(
        const glm::vec2& mouse,
        const Camera& camera,
        const GizmoData& gizmo,
        float viewportWidth, float viewportHeight,
        float threshold)
    {
        const int segments = 32;

        // For each axis, the circle lies in the plane perpendicular to that axis.
        // We sample points along the circle using the other two axes.
        struct CircleDef { GizmoAxis id; GizmoAxis perpA; GizmoAxis perpB; };
        CircleDef circles[] = {
            { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z }, // X rotation: circle in YZ
            { GizmoAxis::Y, GizmoAxis::X, GizmoAxis::Z }, // Y rotation: circle in XZ
            { GizmoAxis::Z, GizmoAxis::X, GizmoAxis::Y }, // Z rotation: circle in XY
        };

        GizmoAxis best = GizmoAxis::None;
        float bestDist = threshold;

        for (auto& circ : circles)
        {
            glm::vec3 dirA = gizmo.GetAxisDir(circ.perpA);
            glm::vec3 dirB = gizmo.GetAxisDir(circ.perpB);

            glm::vec2 prevScreen(0.0f);

            for (int i = 0; i <= segments; ++i)
            {
                float angle = glm::two_pi<float>() * float(i) / float(segments);
                glm::vec3 pointWorld = gizmo.position
                    + dirA * (cosf(angle) * gizmo.axisLength)
                    + dirB * (sinf(angle) * gizmo.axisLength);

                glm::vec2 curScreen = GizmoInteraction::WorldToViewport(
                    pointWorld, camera, viewportWidth, viewportHeight);

                if (i > 0)
                {
                    float dist = DistanceToSegment(mouse, prevScreen, curScreen);
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        best = circ.id;
                    }
                }
                prevScreen = curScreen;
            }
        }
        return best;
    }

    GizmoAxis GizmoInteraction::HitTest(
        const Camera& camera,
        const GizmoData& gizmo,
        float mouseViewportX,
        float mouseViewportY,
        float viewportWidth,
        float viewportHeight)
    {
        const float hitThresholdPx = 12.0f;
        glm::vec2 mouse(mouseViewportX, mouseViewportY);

        if (gizmo.mode == GizmoMode::Rotate)
            return HitTestCircles(mouse, camera, gizmo, viewportWidth, viewportHeight, hitThresholdPx);
        else
            return HitTestLines(mouse, camera, gizmo, viewportWidth, viewportHeight, hitThresholdPx);
    }
}
