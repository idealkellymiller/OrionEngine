#pragma once
#include "EngineCore.h"

#include <string>
#include <vector>


namespace Orion {

	struct Vertex;

	// Result of parsing an OBJ file — geometry plus material references
	struct OBJResult {
		std::vector<Vertex>       vertices;
		std::vector<unsigned int> indices;
		std::string               mtlLibPath;       // resolved path to .mtl file (empty if none)
		std::vector<std::string>  materialNames;    // usemtl names encountered, in order
	};

	class ORION_API OBJLoader
	{
	public:
		// Legacy overload (still works, ignores material info)
		static bool Load(const std::string& filePath,
			std::vector<Vertex>& outVertices,
			std::vector<unsigned int>& outIndices);

		// Full load — returns geometry + material references
		static bool Load(const std::string& filePath, OBJResult& outResult);
	};
}
