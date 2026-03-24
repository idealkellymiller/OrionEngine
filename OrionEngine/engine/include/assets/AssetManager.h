#pragma once
#include <unordered_map>
#include <memory>
#include <string>

#include "AssetTypes.h"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "Material.hpp"


// Manages asset metadata and loaded runtime resources.
class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;

    static void LoadAssetsFolder();

    //// --- Asset registration ---
    //// Adds metadata for a texture asset to the database
    //static void RegisterTextureAsset(const TextureAsset& asset);

    //// Adds metadata for a material asset to the database.
    //static void RegisterMaterialAsset(const MaterialAsset& asset);

    //// Adds metadata for a mesh asset to the database.
    //static void RegisterMeshAsset(const MeshAsset& asset);

    // --- Metadata lookup ---
    static std::shared_ptr<Texture> GetTexture(AssetID assetID);
    static std::shared_ptr<Material> GetMaterial(AssetID assetID);
    static std::shared_ptr<Mesh> GetMesh(AssetID assetID);

    static MeshAsset* GetMeshAsset(AssetID assetID) { return &m_MeshAssets[assetID]; }
    static TextureAsset* GetTextureAsset(AssetID assetID) { return &m_TextureAssets[assetID]; }
    static MaterialAsset* GetMaterialAsset(AssetID assetID) { return &m_MaterialAssets[assetID]; }

    // --- Runtime loading ---
    static void LoadTexture(const std::string filePath);
    static void LoadMesh(const std::string filePath);
    static void LoadMaterial(const std::string filePath);

    static AssetID GetMeshID(const std::string filePath) { return m_MeshPathToID[filePath]; }
    static AssetID GetTextureID(const std::string filePath) { return m_TexturePathToID[filePath]; }
    static AssetID GetMaterialID(const std::string filePath) { return m_MaterialPathToID[filePath]; }

    static void SetAssetsFolderPath(std::string filePath) { m_AssetsFolderPath = filePath; }
    static std::string GetAssetsFolderPath() { return m_AssetsFolderPath; }

private:
    static std::string m_AssetsFolderPath;

    static AssetID m_NextAssetID;

    // Metadata database
    static std::unordered_map<AssetID, MeshAsset>     m_MeshAssets;
    static std::unordered_map<AssetID, TextureAsset>  m_TextureAssets;
    static std::unordered_map<AssetID, MaterialAsset> m_MaterialAssets;

    // Runtime cache
    static std::unordered_map<AssetID, std::shared_ptr<Mesh>>     m_LoadedMeshes;
    static std::unordered_map<AssetID, std::shared_ptr<Texture>>  m_LoadedTextures;
    static std::unordered_map<AssetID, std::shared_ptr<Material>> m_LoadedMaterials;

    // Lookup helpers
    static std::unordered_map<std::string, AssetID> m_MeshPathToID;
    static std::unordered_map<std::string, AssetID> m_TexturePathToID;
    static std::unordered_map<std::string, AssetID> m_MaterialPathToID;
};
