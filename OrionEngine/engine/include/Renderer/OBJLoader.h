#pragma once
#include "EngineCore.h"

#include <string>
#include <vector>


namespace Orion {

	struct Vertex;

	class ORION_API OBJLoader
	{
	public:
		static bool Load(const std::string& filePath,
			std::vector<Vertex>& outVertices,
			std::vector<unsigned int>& outIndices);
	};
}