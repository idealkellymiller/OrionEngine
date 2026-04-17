// <summary>
// This is how and what data assets store in the file directory.
// <summary>
#pragma once
#include "EngineCore.h"
#include <string>
#include <glm/glm.hpp>
#include "Renderer/Texture.h"




namespace Orion {

	using AssetID = uint32_t;
	static constexpr AssetID INVALID_ASSET_ID = 0;


	struct ORION_API MeshAsset {
		AssetID assetID = INVALID_ASSET_ID;
		std::string name = "";
		std::string filePath = "";
	};

	struct ORION_API TextureAsset {
		AssetID assetID = INVALID_ASSET_ID;
		std::string name = "";
		std::string filePath = "";
	};

	struct ORION_API AudioClipAsset {
		AssetID assetID = INVALID_ASSET_ID;
		std::string name = "";
		std::string filePath = "";
	};

	struct ORION_API MaterialAsset {
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
}