#include "AssetManager.h"

#include "OBJLoader.hpp"

#include <iostream>



// --- Asset Registration ---
void AssetManager::RegisterTextureAsset(const TextureAsset& asset)
{
	// Store or overwrite texture asset metadata by AssetID.
	m_TextureAssets[asset.assetID] = asset;
}

void AssetManager::RegisterMaterialAsset(const MaterialAsset& asset)
{
    // Store or overwrite material asset metadata by AssetID.
    m_MaterialAssets[asset.assetID] = asset;
}

void AssetManager::RegisterMeshAsset(const MeshAsset& asset)
{
    // Store or overwrite mesh asset metadata by AssetID.
    m_MeshAssets[asset.assetID] = asset;
}

// --- Metadata lookup ---
TextureAsset* AssetManager::GetTextureAsset(AssetID assetID)
{
    auto it = m_TextureAssets.find(assetID);
    if (it == m_TextureAssets.end())
        return nullptr;

    return &it->second;
}

MaterialAsset* AssetManager::GetMaterialAsset(AssetID assetID)
{
    auto it = m_MaterialAssets.find(assetID);
    if (it == m_MaterialAssets.end())
        return nullptr;

    return &it->second;
}

MeshAsset* AssetManager::GetMeshAsset(AssetID assetID)
{
    auto it = m_MeshAssets.find(assetID);
    if (it == m_MeshAssets.end())
        return nullptr;

    return &it->second;
}

const TextureAsset* AssetManager::GetTextureAsset(AssetID assetID) const
{
    auto it = m_TextureAssets.find(assetID);
    if (it == m_TextureAssets.end())
        return nullptr;

    return &it->second;
}

const MaterialAsset* AssetManager::GetMaterialAsset(AssetID assetID) const
{
    auto it = m_MaterialAssets.find(assetID);
    if (it == m_MaterialAssets.end())
        return nullptr;

    return &it->second;
}

const MeshAsset* AssetManager::GetMeshAsset(AssetID assetID) const
{
    auto it = m_MeshAssets.find(assetID);
    if (it == m_MeshAssets.end())
        return nullptr;

    return &it->second;
}

// --- Utility ---
bool AssetManager::HasTextureAsset(AssetID assetID) const
{
    return m_TextureAssets.find(assetID) != m_TextureAssets.end();
}

bool AssetManager::HasMaterialAsset(AssetID assetID) const
{
    return m_MaterialAssets.find(assetID) != m_MaterialAssets.end();
}

bool AssetManager::HasMeshAsset(AssetID assetID) const
{
    return m_MeshAssets.find(assetID) != m_MeshAssets.end();
}

void AssetManager::Clear()
{
    // Clear metadata database.
    m_TextureAssets.clear();
    m_MaterialAssets.clear();
    m_MeshAssets.clear();

    // Clear loaded runtime cache.
    m_LoadedTextures.clear();
    m_LoadedMeshes.clear();
}


std::shared_ptr<Texture> AssetManager::LoadTexture(AssetID assetID)
{
    // Reject invalid IDs immediately.
    if (assetID == INVALID_ASSET)
        return nullptr;

    // If texture is already loaded, return cached version.
    auto loadedIt = m_LoadedTextures.find(assetID);
    if (loadedIt != m_LoadedTextures.end())
        return loadedIt->second;

    // Look up texture asset metadata.
    TextureAsset* textureAsset = GetTextureAsset(assetID);
    if (!textureAsset) {
        std::cout << "AssetManager::LoadTexture failed - texture asset not found: "
            << assetID << "\n";
        return nullptr;
    }

    // Create the runtime texture object.
    std::shared_ptr<Texture> texture = std::shared_ptr<Texture>();

    // Load texture data from disk into runtime object.
    // This function is assumed to upload image data to OpenGL as part of loading.
    if (!texture->LoadFromFile(textureAsset->filePath)) {
        std::cout << "AssetManager::LoadTexture failed - could not load file: "
            << textureAsset->filePath << "\n";
        return nullptr;
    }

    // Cachee the loaded texture so future requests reuse it.
    m_LoadedTextures[assetID] = texture;

    return texture;
}

std::shared_ptr<Mesh> AssetManager::LoadMesh(AssetID assetID)
{
    // Reject invalid IDs immediately.
    if (assetID == INVALID_ASSET)
        return nullptr;

    // If mesh is already loaded, return cached version.
    auto loadedIt = m_LoadedMeshes.find(assetID);
    if (loadedIt != m_LoadedMeshes.end())
        return loadedIt->second;

    // Look up mesh asset metadata.
    MeshAsset* meshAsset = GetMeshAsset(assetID);
    if (!meshAsset)
    {
        std::cout << "AssetManager::LoadMesh failed - mesh asset not found: "
            << assetID << "\n";
        return nullptr;
    }

    // Load CPU-side mesh data from the source file.
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    if (!OBJLoader::Load(meshAsset->filePath, vertices, indices))
    {
        std::cout << "AssetManager::LoadMesh failed - could not load mesh file: "
            << meshAsset->filePath << "\n";
        return nullptr;
    }

    std::shared_ptr<Mesh> mesh = std::shared_ptr<Mesh>();

    // Upload vertex/indices data to gpu buffers.
    mesh->Create(vertices, indices);

    // cache the loaded mesh for reuse.
    m_LoadedMeshes[assetID] = mesh;

    return mesh;
}