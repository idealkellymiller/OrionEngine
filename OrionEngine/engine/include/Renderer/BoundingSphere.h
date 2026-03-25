#pragma once
#include <glm/glm.hpp>


struct BoundingSphere {
	glm::vec3 Center = glm::vec3(0.0f);	// Local-space center of the sphere
	float Radius = 0.0f;				// Local-space radius
};