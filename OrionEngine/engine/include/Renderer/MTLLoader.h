#pragma once
#include "EngineCore.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Orion {

	// One material entry parsed from a Wavefront .mtl file
	struct MTLMaterial {
		std::string name;

		glm::vec3 diffuseColor  = { 0.8f, 0.8f, 0.8f };  // Kd
		glm::vec3 specularColor = { 1.0f, 1.0f, 1.0f };  // Ks
		float     shininess     = 32.0f;                    // Ns (specular exponent)
		float     opacity       = 1.0f;                     // d (1 = opaque)

		std::string diffuseMap;    // map_Kd (texture path, relative to .mtl location)
	};

	class ORION_API MTLLoader {
	public:
		// Parse a .mtl file and return all material definitions found in it.
		// Texture paths in the results are resolved to absolute paths.
		static bool Load(const std::string& filePath, std::vector<MTLMaterial>& outMaterials);
	};
}
