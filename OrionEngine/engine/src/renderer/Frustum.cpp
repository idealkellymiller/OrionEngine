#include "Frustum.hpp"
#include <glm/glm.hpp>
#include <cmath>


void Frustum::Build(const glm::mat4 viewProjection)
{
	// glm matrices are column-major.
	// We extact planes from the transposed pattern manually.

    // Left plane
    m_Planes[0].Normal.x = viewProjection[0][3] + viewProjection[0][0];
    m_Planes[0].Normal.y = viewProjection[1][3] + viewProjection[1][0];
    m_Planes[0].Normal.z = viewProjection[2][3] + viewProjection[2][0];
    m_Planes[0].Distance = viewProjection[3][3] + viewProjection[3][0];

    // Right plane
    m_Planes[1].Normal.x = viewProjection[0][3] - viewProjection[0][0];
    m_Planes[1].Normal.y = viewProjection[1][3] - viewProjection[1][0];
    m_Planes[1].Normal.z = viewProjection[2][3] - viewProjection[2][0];
    m_Planes[1].Distance = viewProjection[3][3] - viewProjection[3][0];

    // Bottom plane
    m_Planes[2].Normal.x = viewProjection[0][3] + viewProjection[0][1];
    m_Planes[2].Normal.y = viewProjection[1][3] + viewProjection[1][1];
    m_Planes[2].Normal.z = viewProjection[2][3] + viewProjection[2][1];
    m_Planes[2].Distance = viewProjection[3][3] + viewProjection[3][1];

    // Top plane
    m_Planes[3].Normal.x = viewProjection[0][3] - viewProjection[0][1];
    m_Planes[3].Normal.y = viewProjection[1][3] - viewProjection[1][1];
    m_Planes[3].Normal.z = viewProjection[2][3] - viewProjection[2][1];
    m_Planes[3].Distance = viewProjection[3][3] - viewProjection[3][1];

    // Near plane
    m_Planes[4].Normal.x = viewProjection[0][3] + viewProjection[0][2];
    m_Planes[4].Normal.y = viewProjection[1][3] + viewProjection[1][2];
    m_Planes[4].Normal.z = viewProjection[2][3] + viewProjection[2][2];
    m_Planes[4].Distance = viewProjection[3][3] + viewProjection[3][2];

    // Far plane
    m_Planes[5].Normal.x = viewProjection[0][3] - viewProjection[0][2];
    m_Planes[5].Normal.y = viewProjection[1][3] - viewProjection[1][2];
    m_Planes[5].Normal.z = viewProjection[2][3] - viewProjection[2][2];
    m_Planes[5].Distance = viewProjection[3][3] - viewProjection[3][2];

    // Normalize all planes so distance checks are correct
    for (FrustumPlane& plane : m_Planes) {
        NormalizePlane(plane);
    }
}

void Frustum::NormalizePlane(FrustumPlane& plane)
{
    float length = glm::length(plane.Normal);
    if (length == 0.0f)
        return;

    plane.Normal /= length;
    plane.Distance /= length;
}

bool Frustum::IntersectsSphere(const glm::vec3& center, float radius) const
{
    for (const FrustumPlane& plane : m_Planes) {
        // Signed distance from sphere to center to plane.
        float distance = glm::dot(plane.Normal, center) + plane.Distance;

        // If sphere is fully behind any plane, it is outside the frustum.
        if (distance < -radius) {
            return false;
        }
    }

    return true;
}