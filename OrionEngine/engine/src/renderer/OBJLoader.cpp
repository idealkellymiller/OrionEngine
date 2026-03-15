#include "OBJLoader.hpp"
#include "Vertex.hpp"

#include <glm/glm.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>


struct OBJIndex
{
	int PositionIndex = 0;
	int UVIndex = 0;
	int NormalIndex = 0;

	bool operator==(const OBJIndex& other) const
	{
		return PositionIndex == other.PositionIndex &&
			UVIndex == other.UVIndex &&
			NormalIndex == other.NormalIndex;
	}
};

struct OBJIndexHash
{
	size_t operator()(const OBJIndex& index) const
	{
		size_t h1 = std::hash<int>()(index.PositionIndex);
		size_t h2 = std::hash<int>()(index.UVIndex);
		size_t h3 = std::hash<int>()(index.NormalIndex);

		// Combine hashes
		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};


bool ParseFaceVertex(const std::string& token, OBJIndex& outIndex)
{
	// Expected token format:
	// position/uv/normal
	// Example: 3/2/1

	std::stringstream ss(token);
	std::string positionStr;
	std::string uvStr;
	std::string normalStr;

	if (!std::getline(ss, positionStr, '/'))
		return false;

	if (!std::getline(ss, uvStr, '/'))
		return false;

	if (!std::getline(ss, normalStr, '/'))
		return false;

	// Convert strings to integers
	outIndex.PositionIndex = std::stoi(positionStr);
	outIndex.UVIndex = std::stoi(uvStr);
	outIndex.NormalIndex = std::stoi(normalStr);

	return true;
}


bool OBJLoader::Load(const std::string& filePath, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices)
{
	outVertices.clear();
	outIndices.clear();

	std::ifstream file(filePath);
	if (!file.is_open())
	{
		std::cout << "Failed to open OBJ file: " << filePath << "\n";
		return false;
	}

	// Raw OBJ data
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	// This maps a unique OBJ face index triplet (pos/uv/normal)
	// to the final vertex index in outVertices.
	std::unordered_map<OBJIndex, unsigned int, OBJIndexHash> vertexLookup;

	std::string line;
	while (std::getline(file, line))
	{
		// Skip empty lines
		if (line.empty())
			continue;

		// Skip comments
		if (line[0] == '#')
			continue;

		std::stringstream ss(line);
		std::string prefix;
		ss >> prefix;

		if (prefix == "v")
		{
			// Position line: v x y z
			glm::vec3 position;
			ss >> position.x >> position.y >> position.z;
			positions.push_back(position);
		}
		else if (prefix == "vt")
		{
			// UV line: vt u v
			glm::vec2 uv;
			ss >> uv.x >> uv.y;
			uvs.push_back(uv);
		}
		else if (prefix == "vn")
		{
			// Normal line: vn x y z
			glm::vec3 normal;
			ss >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (prefix == "f")
		{
			// Only supporting triangles for now:
			// f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3

			std::string token0;
			std::string token1;
			std::string token2;

			ss >> token0 >> token1 >> token2;

			if (token0.empty() || token1.empty() || token2.empty())
			{
				std::cout << "Invalid face line in OBJ: " << line << "\n";
				return false;
			}

			OBJIndex i0;
			OBJIndex i1;
			OBJIndex i2;

			if (!ParseFaceVertex(token0, i0) ||
				!ParseFaceVertex(token1, i1) ||
				!ParseFaceVertex(token2, i2))
			{
				std::cout << "Failed to parse face indices in OBJ: " << line << "\n";
				return false;
			}

			OBJIndex faceIndices[3] = { i0, i1, i2 };

			for (int i = 0; i < 3; ++i)
			{
				const OBJIndex& objIndex = faceIndices[i];

				// First check if we've already created this exact vertex
				auto it = vertexLookup.find(objIndex);
				if (it != vertexLookup.end())
				{
					// Reuse existing vertex index
					outIndices.push_back(it->second);
					continue;
				}

				// OBJ uses 1-based indices, convert to 0-based
				int positionIndex = objIndex.PositionIndex - 1;
				int uvIndex = objIndex.UVIndex - 1;
				int normalIndex = objIndex.NormalIndex - 1;

				// Validate indices
				if (positionIndex < 0 || positionIndex >= (int)positions.size())
				{
					std::cout << "OBJ position index out of range in file: " << filePath << "\n";
					return false;
				}

				if (uvIndex < 0 || uvIndex >= (int)uvs.size())
				{
					std::cout << "OBJ UV index out of range in file: " << filePath << "\n";
					return false;
				}

				if (normalIndex < 0 || normalIndex >= (int)normals.size())
				{
					std::cout << "OBJ normal index out of range in file: " << filePath << "\n";
					return false;
				}

				// Build the final engine vertex
				Vertex vertex;
				vertex.Position = positions[positionIndex];
				vertex.UV = uvs[uvIndex];
				vertex.Normal = normals[normalIndex];

				// New vertex index is current size before pushing
				unsigned int newVertexIndex = (unsigned int)outVertices.size();

				outVertices.push_back(vertex);
				outIndices.push_back(newVertexIndex);

				// Store in lookup so later faces can reuse it
				vertexLookup[objIndex] = newVertexIndex;
			}
		}
		else
		{
			// Ignore unsupported lines for now:
			// o, g, s, mtllib, usemtl, etc.
		}
	}

	if (outVertices.empty())
	{
		std::cout << "OBJ file produced no vertices: " << filePath << "\n";
		return false;
	}

	std::cout << "OBJ loaded successfully: " << filePath << "\n";
	std::cout << "  Vertices: " << outVertices.size() << "\n";
	std::cout << "  Indices: " << outIndices.size() << "\n";

	return true;
}