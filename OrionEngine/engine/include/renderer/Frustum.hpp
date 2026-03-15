// A frustum can be represented by 6 planes: left, right, bottom, top, near, far.
#pragma once
#include <array>
#include <glm/glm.hpp>

#include "BoundingSphere.hpp"


// A plane in the form: ax + by + cz + d = 0
struct FrustumPlane {
	glm::vec3 Normal = glm::vec3(0.0f, 1.0f, 0.0f);
	float Distance = 0.0f;
};

class Frustum {
public:
	// Extract frustum planes from a combined projection * view matrix
	void Build(const glm::mat4 viewProjection);

	// Returns true of the sphere is at least partially inside of the frustum
	bool IntersectsSphere(const glm::vec3& center, float radius) const;

private:
	// Normalize a plane so its normal has length 1.
	void NormalizePlane(FrustumPlane& plane);

private:
	// 6 standard frustum planes.
	std::array<FrustumPlane, 6> m_Planes;
};