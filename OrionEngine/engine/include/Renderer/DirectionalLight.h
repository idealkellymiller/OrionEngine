#pragma once


#include <glm/glm.hpp>


struct DirectionalLight {
	// Direction the light travels in world space.
	// Example: (0, -1, 0) means light shining downward.
	glm::vec3 Direction = glm::vec3(-0.2f, -1.0f, -0.3f);

	// FRG light color
	glm::vec3 Color = glm::vec3(1.0f, 1.0f, 1.0f);

	// Scalar brightness multiplier.
	float Intensity = 1.0f;
};