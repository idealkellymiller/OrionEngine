#pragma once
#include "EngineCore.h"

#include <glm/glm.hpp>

namespace Orion {

	struct ORION_API DirectionalLight {
		// Direction the light travels in world space.
		// Example: (0, -1, 0) means light shining downward.
		glm::vec3 Direction = glm::vec3(-0.2f, -1.0f, -0.3f);

		// RGB light color
		glm::vec3 Color = glm::vec3(1.0f, 1.0f, 1.0f);

		// Scalar brightness multiplier.
		float Intensity = 1.0f;
	};

	// Maximum number of point lights the shader supports.
	static constexpr int MAX_POINT_LIGHTS = 16;

	struct ORION_API PointLight {
		glm::vec3 Position = glm::vec3(0.0f);
		glm::vec3 Color    = glm::vec3(1.0f, 1.0f, 1.0f);
		float Intensity    = 1.0f;

		// Attenuation: 1.0 / (constant + linear*d + quadratic*d^2)
		float Constant     = 1.0f;
		float Linear       = 0.09f;
		float Quadratic    = 0.032f;
	};
}