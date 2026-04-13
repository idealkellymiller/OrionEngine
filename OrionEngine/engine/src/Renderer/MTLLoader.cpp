#include "Renderer/MTLLoader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace Orion {

	bool MTLLoader::Load(const std::string& filePath, std::vector<MTLMaterial>& outMaterials)
	{
		outMaterials.clear();

		std::ifstream file(filePath);
		if (!file.is_open()) {
			std::cout << "[MTLLoader] Failed to open: " << filePath << "\n";
			return false;
		}

		std::filesystem::path mtlDir = std::filesystem::path(filePath).parent_path();

		MTLMaterial* current = nullptr;

		std::string line;
		while (std::getline(file, line))
		{
			// Trim trailing \r
			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			if (line.empty() || line[0] == '#')
				continue;

			std::stringstream ss(line);
			std::string tag;
			ss >> tag;

			if (tag == "newmtl")
			{
				// Start a new material
				std::string name;
				std::getline(ss >> std::ws, name);
				while (!name.empty() && (name.back() == ' ' || name.back() == '\r'))
					name.pop_back();

				outMaterials.push_back({});
				current = &outMaterials.back();
				current->name = name;
			}
			else if (!current)
			{
				// No active material yet, skip
				continue;
			}
			else if (tag == "Kd")
			{
				// Diffuse color
				ss >> current->diffuseColor.r >> current->diffuseColor.g >> current->diffuseColor.b;
			}
			else if (tag == "Ks")
			{
				// Specular color
				ss >> current->specularColor.r >> current->specularColor.g >> current->specularColor.b;
			}
			else if (tag == "Ns")
			{
				// Specular exponent (OBJ typically 0-1000, we use directly)
				ss >> current->shininess;
				// Clamp to reasonable range
				if (current->shininess < 1.0f) current->shininess = 1.0f;
				if (current->shininess > 512.0f) current->shininess = 512.0f;
			}
			else if (tag == "d")
			{
				// Dissolve (opacity): 1 = fully opaque, 0 = transparent
				ss >> current->opacity;
			}
			else if (tag == "Tr")
			{
				// Transparency (inverse of d): 0 = opaque, 1 = transparent
				float tr;
				ss >> tr;
				current->opacity = 1.0f - tr;
			}
			else if (tag == "map_Kd")
			{
				// Diffuse texture map
				std::string texPath;
				std::getline(ss >> std::ws, texPath);
				while (!texPath.empty() && (texPath.back() == ' ' || texPath.back() == '\r'))
					texPath.pop_back();

				if (!texPath.empty()) {
					// Try relative to .mtl file first (Blender convention)
					std::filesystem::path resolved = mtlDir / texPath;
					if (std::filesystem::exists(resolved)) {
						current->diffuseMap = resolved.string();
					}
					else {
						// Fall back: search all common image extensions with same stem
						// in case the path is relative to the assets root instead
						current->diffuseMap = texPath; // store raw, AssetManager will resolve
					}
				}
			}
			// Silently ignore: Ka, Ke, Ni, illum, map_Ks, map_Bump, etc.
		}

		std::cout << "[MTLLoader] Parsed " << filePath << " | " << outMaterials.size() << " material(s)\n";
		for (auto& m : outMaterials)
			std::cout << "  - " << m.name
				<< " Kd=(" << m.diffuseColor.r << "," << m.diffuseColor.g << "," << m.diffuseColor.b << ")"
				<< (m.diffuseMap.empty() ? "" : " map_Kd=" + m.diffuseMap)
				<< "\n";

		return true;
	}
}
