#include "Renderer/OBJLoader.h"
#include "Renderer/Vertex.h"

#include <glm/glm.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace Orion {

	struct OBJIndex
	{
		int PositionIndex = -1;
		int UVIndex       = -1;
		int NormalIndex    = -1;

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
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	// Parse a single face vertex token.
	// Handles these formats from Blender exports:
	//   v/vt/vn   (e.g. 3/2/1)
	//   v//vn     (no UVs)
	//   v/vt      (no normals)
	//   v         (position only)
	static bool ParseFaceVertex(const std::string& token, OBJIndex& outIndex)
	{
		outIndex = {};

		if (token.empty())
			return false;

		// Split on '/'
		std::vector<std::string> parts;
		std::stringstream ss(token);
		std::string part;
		while (std::getline(ss, part, '/'))
			parts.push_back(part);

		if (parts.empty())
			return false;

		// Position (always required)
		outIndex.PositionIndex = std::stoi(parts[0]);

		// UV (optional)
		if (parts.size() >= 2 && !parts[1].empty())
			outIndex.UVIndex = std::stoi(parts[1]);

		// Normal (optional)
		if (parts.size() >= 3 && !parts[2].empty())
			outIndex.NormalIndex = std::stoi(parts[2]);

		return true;
	}

	// Core loader implementation
	bool OBJLoader::Load(const std::string& filePath, OBJResult& outResult)
	{
		outResult = {};

		std::ifstream file(filePath);
		if (!file.is_open())
		{
			std::cout << "[OBJLoader] Failed to open: " << filePath << "\n";
			return false;
		}

		// Resolve directory for relative mtllib paths
		std::filesystem::path objDir = std::filesystem::path(filePath).parent_path();

		// Raw OBJ data pools
		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> normals;

		// Deduplication map
		std::unordered_map<OBJIndex, unsigned int, OBJIndexHash> vertexLookup;

		auto emitVertex = [&](const OBJIndex& objIndex) -> unsigned int
		{
			// Check cache first
			auto it = vertexLookup.find(objIndex);
			if (it != vertexLookup.end())
				return it->second;

			Vertex vertex;

			// Position (required, 1-based)
			int pi = objIndex.PositionIndex;
			if (pi < 0) pi = (int)positions.size() + pi + 1; // negative = relative
			pi -= 1; // to 0-based
			if (pi >= 0 && pi < (int)positions.size())
				vertex.Position = positions[pi];

			// UV (optional)
			if (objIndex.UVIndex != -1) {
				int ui = objIndex.UVIndex;
				if (ui < 0) ui = (int)uvs.size() + ui + 1;
				ui -= 1;
				if (ui >= 0 && ui < (int)uvs.size())
					vertex.UV = uvs[ui];
			}

			// Normal (optional)
			if (objIndex.NormalIndex != -1) {
				int ni = objIndex.NormalIndex;
				if (ni < 0) ni = (int)normals.size() + ni + 1;
				ni -= 1;
				if (ni >= 0 && ni < (int)normals.size())
					vertex.Normal = normals[ni];
			}

			unsigned int newIndex = (unsigned int)outResult.vertices.size();
			outResult.vertices.push_back(vertex);
			vertexLookup[objIndex] = newIndex;
			return newIndex;
		};

		std::string line;
		while (std::getline(file, line))
		{
			// Trim trailing \r (Windows line endings in files)
			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			if (line.empty() || line[0] == '#')
				continue;

			std::stringstream ss(line);
			std::string prefix;
			ss >> prefix;

			if (prefix == "v")
			{
				glm::vec3 p;
				ss >> p.x >> p.y >> p.z;
				positions.push_back(p);
			}
			else if (prefix == "vt")
			{
				glm::vec2 uv;
				ss >> uv.x >> uv.y;
				uvs.push_back(uv);
			}
			else if (prefix == "vn")
			{
				glm::vec3 n;
				ss >> n.x >> n.y >> n.z;
				normals.push_back(n);
			}
			else if (prefix == "f")
			{
				// Collect all face vertex tokens (supports tris, quads, n-gons)
				std::vector<OBJIndex> faceVerts;
				std::string token;
				while (ss >> token) {
					OBJIndex idx;
					if (!ParseFaceVertex(token, idx)) {
						std::cout << "[OBJLoader] Bad face token '" << token << "' in: " << line << "\n";
						return false;
					}
					faceVerts.push_back(idx);
				}

				if (faceVerts.size() < 3) {
					std::cout << "[OBJLoader] Face with < 3 verts in: " << line << "\n";
					return false;
				}

				// Fan triangulation: (0,1,2), (0,2,3), (0,3,4), ...
				unsigned int v0 = emitVertex(faceVerts[0]);
				for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
					unsigned int v1 = emitVertex(faceVerts[i]);
					unsigned int v2 = emitVertex(faceVerts[i + 1]);
					outResult.indices.push_back(v0);
					outResult.indices.push_back(v1);
					outResult.indices.push_back(v2);
				}
			}
			else if (prefix == "mtllib")
			{
				std::string mtlFile;
				std::getline(ss >> std::ws, mtlFile); // rest of line (may have spaces)
				// Trim trailing whitespace/CR
				while (!mtlFile.empty() && (mtlFile.back() == ' ' || mtlFile.back() == '\r' || mtlFile.back() == '\t'))
					mtlFile.pop_back();

				std::filesystem::path mtlPath = objDir / mtlFile;
				outResult.mtlLibPath = mtlPath.string();
			}
			else if (prefix == "usemtl")
			{
				std::string matName;
				std::getline(ss >> std::ws, matName);
				while (!matName.empty() && (matName.back() == ' ' || matName.back() == '\r' || matName.back() == '\t'))
					matName.pop_back();

				// Track unique material names in order
				bool found = false;
				for (auto& n : outResult.materialNames)
					if (n == matName) { found = true; break; }
				if (!found)
					outResult.materialNames.push_back(matName);
			}
			// else: o, g, s, l — silently ignore
		}

		if (outResult.vertices.empty())
		{
			std::cout << "[OBJLoader] File produced no vertices: " << filePath << "\n";
			return false;
		}

		std::cout << "[OBJLoader] Loaded " << filePath
			<< " | Verts: " << outResult.vertices.size()
			<< " | Tris: " << (outResult.indices.size() / 3)
			<< " | Materials: " << outResult.materialNames.size();
		if (!outResult.mtlLibPath.empty())
			std::cout << " | MTL: " << outResult.mtlLibPath;
		std::cout << "\n";

		return true;
	}

	// Legacy overload — delegates to full loader
	bool OBJLoader::Load(const std::string& filePath, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices)
	{
		OBJResult result;
		if (!Load(filePath, result))
			return false;
		outVertices = std::move(result.vertices);
		outIndices = std::move(result.indices);
		return true;
	}
}
