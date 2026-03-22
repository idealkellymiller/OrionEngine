// <summary>
// This is how and what data assets store in the file directory.
// <summary>
#pragma once
#include <string>
#include <glm/glm.hpp>


using AssetID = uint32_t;
static constexpr AssetID INVALID_ASSET = 0;


struct MaterialAsset {
	AssetID assetID = INVALID_ASSET;
	std::string name = "";
	AssetID diffuseTexture = INVALID_ASSET;
	glm::vec4 colorTint = glm::vec4(1.0f);
	glm::vec3 specularColor = glm::vec3(1.0f);
	float specularShininess = 10.0f;
	bool isTransparent = false;
};

struct TextureAsset {
	AssetID assetID = INVALID_ASSET;
	std::string name = "";
	std::string filePath = "";
};

struct MeshAsset {
	AssetID assetID = INVALID_ASSET;
	std::string name = "";
	std::string filePath = "";
	AssetID material = INVALID_ASSET;
};