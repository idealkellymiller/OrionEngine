#include "Assets/AssetManager.h"
#include "Renderer/OBJLoader.h"

#include <iostream>
#include <filesystem>
#include <string>

namespace Orion {

    std::string AssetManager::m_AssetsFolderPath = "";

    AssetID AssetManager::m_NextAssetID = 1;

    std::unordered_map<AssetID, MeshAsset> AssetManager::m_MeshAssets;
    std::unordered_map<AssetID, TextureAsset> AssetManager::m_TextureAssets;
    std::unordered_map<AssetID, MaterialAsset> AssetManager::m_MaterialAssets;

    std::unordered_map<AssetID, std::shared_ptr<Mesh>> AssetManager::m_LoadedMeshes;
    std::unordered_map<AssetID, std::shared_ptr<Texture>> AssetManager::m_LoadedTextures;
    std::unordered_map<AssetID, std::shared_ptr<Material>> AssetManager::m_LoadedMaterials;

    std::unordered_map<std::string, AssetID> AssetManager::m_MeshPathToID;
    std::unordered_map<std::string, AssetID> AssetManager::m_TexturePathToID;
    std::unordered_map<std::string, AssetID> AssetManager::m_MaterialPathToID;


    namespace fs = std::filesystem;
    void AssetManager::LoadAssetsFolder()
    {

        std::cout << "m_AssetsFolderPath = " << m_AssetsFolderPath << "\n";
        std::cout << "Current working directory = " << fs::current_path().string() << "\n";

        if (m_AssetsFolderPath.empty())
        {
            std::cout << "Asset folder path is empty.\n";
            return;
        }

        fs::path assetRoot = m_AssetsFolderPath;


        if (!fs::exists(assetRoot) || !fs::is_directory(assetRoot))
        {
            std::cout << "Invalid asset folder path: " << assetRoot.string() << "\n";
            return;
        }

        // Collect first, load second (to allow for textures to be loaded before mats)
        std::vector<fs::path> meshFiles;
        std::vector<fs::path> textureFiles;
        std::vector<fs::path> materialFiles;

        // Recursively walk trhough every file/folder under asset root.
        for (const auto& entry : fs::recursive_directory_iterator(assetRoot)) {
            // Skip anything that is not a regular file
            if (!entry.is_regular_file())
                continue;

            const fs::path& filePath = entry.path();
            std::string extension = filePath.extension().string();

            // Normalize to lowercase just in case files are .PNG, etc
            for (char& c : extension)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (extension == ".obj")
            {
                meshFiles.push_back(filePath);
            }
            else if (extension == ".png")
            {
                textureFiles.push_back(filePath);
            }
            else if (extension == ".mtrl")
            {
                materialFiles.push_back(filePath);
            }
            else {
                std::cout << "Invalid file while loading assets folder: " << extension << "\n";
            }
        }

        // Optional: sort alphabetically for deterministic loading within each group
        std::sort(meshFiles.begin(), meshFiles.end());
        std::sort(textureFiles.begin(), textureFiles.end());
        std::sort(materialFiles.begin(), materialFiles.end());


        // Load in your chosen order
        for (const auto& path : meshFiles)
            LoadMesh(path.string());

        for (const auto& path : textureFiles)
            LoadTexture(path.string());

        for (const auto& path : materialFiles)
            LoadMaterial(path.string());

        std::cout << "\nSuccessfully loaded assets.\n";
    }





    std::shared_ptr<Mesh> AssetManager::GetMesh(AssetID assetID)
    {
        auto it = m_LoadedMeshes.find(assetID);
        if (it != m_LoadedMeshes.end())
            return it->second;

        return nullptr;
    }

    std::shared_ptr<Texture> AssetManager::GetTexture(AssetID assetID)
    {
        auto it = m_LoadedTextures.find(assetID);
        if (it != m_LoadedTextures.end())
            return it->second;

        return nullptr;
    }

    std::shared_ptr<Material> AssetManager::GetMaterial(AssetID assetID)
    {
        auto it = m_LoadedMaterials.find(assetID);
        if (it != m_LoadedMaterials.end())
            return it->second;

        return nullptr;
    }





    void AssetManager::LoadMesh(const std::string filePath)
    {
        // Avoid duplicate loads
        auto existing = m_MeshPathToID.find(filePath);
        if (existing != m_MeshPathToID.end())
            return;

        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        if (!OBJLoader::Load(filePath, vertices, indices)) {
            std::cout << "Failed to parse OBJ: " << filePath << "\n";
            return;
        }

        if (!mesh->Create(vertices, indices)) {
            std::cout << "Failed to load mesh: " << filePath << "\n";
            return;
        }

        AssetID assetID = m_NextAssetID++;

        MeshAsset asset;
        asset.assetID = assetID;
        asset.filePath = filePath;

        std::filesystem::path path(filePath);   // convert to Path to use .stem()
        asset.name = path.stem().string();

        // Store metadata
        m_MeshAssets[assetID] = asset;
        // Store runtime object
        m_LoadedMeshes[assetID] = mesh;
        // Store reverse lookup
        m_MeshPathToID[filePath] = assetID;
    }

    void AssetManager::LoadTexture(const std::string filePath)
    {
        // Avoid duplicate loads
        auto existing = m_TexturePathToID.find(filePath);
        if (existing != m_TexturePathToID.end())
            return;

        // Create texture on heap (shared ownership)
        std::shared_ptr<Texture> texture = std::make_shared<Texture>();

        if (!texture->LoadFromFile(filePath)) {
            std::cout << "Failed to load texture: " << filePath << "\n";
            return;
        }

        AssetID assetID = m_NextAssetID++;

        TextureAsset asset;
        asset.assetID = assetID;
        asset.filePath = filePath;
        std::filesystem::path path(filePath);   // convert to Path to use .stem()
        asset.name = path.stem().string();

        // Store metadata
        m_TextureAssets[assetID] = asset;
        // Store runtime object
        m_LoadedTextures[assetID] = texture;
        // Store reverse lookup
        m_TexturePathToID[filePath] = assetID;
    }

    void AssetManager::LoadMaterial(const std::string filePath)
    {
        // Avoid duplicate loads
        auto existing = m_MaterialPathToID.find(filePath);
        if (existing != m_MaterialPathToID.end())
            return;

        std::shared_ptr<Material> material = std::make_shared<Material>();

        if (!material->LoadFromFile(filePath)) {
            std::cout << "Failed to load material: " << filePath << "\n";
            return;
        }

        AssetID assetID = m_NextAssetID++;

        MaterialAsset asset;
        asset.assetID = assetID;
        asset.filePath = filePath;

        std::filesystem::path path(filePath);   // convert to Path to use .stem()
        asset.name = path.stem().string();

        asset.colorTint = material->GetColor();
        asset.specularColor = material->GetSpecularColor();
        asset.specularShininess = material->GetShininess();
        asset.isTransparent = material->IsTransparent();

        // asset.diffuseTexture = 
        // Get texture id based on stored path in
        // AssetID textureID = m_TexturePathToID[]
        AssetID textureID = GetTextureID(material->GetDiffusePath());
        asset.diffuseTexture = *GetTextureAsset(textureID);

        // Store metadata
        m_MaterialAssets[assetID] = asset;
        // Store runtime object
        m_LoadedMaterials[assetID] = material;
        // Store reverse lookup
        m_MaterialPathToID[filePath] = assetID;
    }
}