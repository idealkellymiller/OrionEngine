#pragma once

#include <string>
#include <vector>

struct Vertex;

class OBJLoader
{
public:
	static bool Load(const std::string& filePath,
		std::vector<Vertex>& outVertices,
		std::vector<unsigned int>& outIndices);
};