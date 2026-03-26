#pragma once
#include "EngineCore.h"
#include <glm/glm.hpp>


namespace Orion {

	struct ORION_API BoundingSphere {
		glm::vec3 Center = glm::vec3(0.0f);	// Local-space center of the sphere
		float Radius = 0.0f;				// Local-space radius
	};
}