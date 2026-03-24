// <summary>
// This is how and what data assets store in the file directory.
// <summary>
#pragma once
#include <string>
#include <glm/glm.hpp>
#include "Texture.hpp"


using AssetID = uint32_t;
static constexpr AssetID INVALID_ASSET_ID = 0;


struct MeshAsset {
	AssetID assetID = INVALID_ASSET_ID;
	std::string name = "";
	std::string filePath = "";
};

struct TextureAsset {
	AssetID assetID = INVALID_ASSET_ID;
	std::string name = "";
	std::string filePath = "";
};

struct MaterialAsset {
	AssetID assetID = INVALID_ASSET_ID;
	std::string name = "";
	std::string filePath = "";

	// Reference, not embedded object
	TextureAsset diffuseTexture;

	glm::vec4 colorTint = glm::vec4(1.0f);
	glm::vec3 specularColor = glm::vec3(1.0f);
	float specularShininess = 10.0f;
	bool isTransparent = false;
};