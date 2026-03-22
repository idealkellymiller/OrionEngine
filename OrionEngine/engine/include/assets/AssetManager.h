#pragma once
#include <unordered_map>
#include <memory>
#include <string>

#include "AssetTypes.h"
#include "Mesh.hpp"
#include "Texture.hpp"


// Manages asset metadata and loaded runtime resources.
class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;

    // --- Asset registration ---
    // Adds metadata for a texture asset to the database
    void RegisterTextureAsset(const TextureAsset& asset);

    // Adds metadata for a material asset to the database.
    void RegisterMaterialAsset(const MaterialAsset& asset);

    // Adds metadata for a mesh asset to the database.
    void RegisterMeshAsset(const MeshAsset& asset);

    // --- Metadata lookup ---
    TextureAsset* GetTextureAsset(AssetID assetID);
    MaterialAsset* GetMaterialAsset(AssetID assetID);
    MeshAsset* GetMeshAsset(AssetID assetID);

    const TextureAsset* GetTextureAsset(AssetID assetID) const;
    const MaterialAsset* GetMaterialAsset(AssetID assetID) const;
    const MeshAsset* GetMeshAsset(AssetID assetID) const;

    // --- Runtime loading ---
    // Loads the texture if needed, then returns the runtime texture object.
    std::shared_ptr<Texture> LoadTexture(AssetID assetID);

    // Loads the mesh if needed, then returns the runtime mesh object.
    std::shared_ptr<Mesh> LoadMesh(AssetID assetID);

    // --- Utility ---
    bool HasTextureAsset(AssetID assetID) const;
    bool HasMaterialAsset(AssetID assetID) const;
    bool HasMeshAsset(AssetID assetID) const;

    void Clear();

private:
    // Metadata database
    std::unordered_map<AssetID, TextureAsset> m_TextureAssets;
    std::unordered_map<AssetID, MaterialAsset> m_MaterialAssets;
    std::unordered_map<AssetID, MeshAsset> m_MeshAssets;

    // Loaded runtime cache
    std::unordered_map<AssetID, std::shared_ptr<Texture>> m_LoadedTextures;
    std::unordered_map<AssetID, std::shared_ptr<Mesh>> m_LoadedMeshes;
};
