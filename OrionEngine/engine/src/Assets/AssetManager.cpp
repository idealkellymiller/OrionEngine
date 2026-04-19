#include "Assets/AssetManager.h"
#include "Renderer/OBJLoader.h"
#include "Renderer/MTLLoader.h"
#include "Renderer/Renderer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <algorithm>

namespace Orion {

    std::string AssetManager::m_AssetsFolderPath = "";
    std::string AssetManager::m_EngineAssetsFolderPath = "";

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
    std::unordered_map<std::string, AssetID> AssetManager::m_AudioClipPathToID;

    std::unordered_map<AssetID, AudioClipAsset> AssetManager::m_AudioClipAssets;


    // Helper: is this extension a supported image format?
    static bool IsTextureExtension(const std::string& ext)
    {
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg"
            || ext == ".tga" || ext == ".bmp" || ext == ".hdr";
    }

    // Helper: is this extension a supported audio format?
    static bool IsAudioExtension(const std::string& ext)
    {
        return ext == ".wav" || ext == ".mp3" || ext == ".ogg"
            || ext == ".flac" || ext == ".opus";
    }

    namespace fs = std::filesystem;

    void AssetManager::LoadFolder(const std::string& folderPath)
    {
        if (folderPath.empty())
        {
            std::cout << "[Assets] LoadFolder: path is empty.\n";
            return;
        }

        fs::path assetRoot = folderPath;

        if (!fs::exists(assetRoot) || !fs::is_directory(assetRoot))
        {
            std::cout << "[Assets] LoadFolder: invalid directory: " << assetRoot.string() << "\n";
            return;
        }

        // Collect first, load second
        std::vector<fs::path> meshFiles;
        std::vector<fs::path> textureFiles;
        std::vector<fs::path> mtlFiles;
        std::vector<fs::path> mtrlFiles;
        std::vector<fs::path> audioFiles;

        // Recursively walk through every file/folder under asset root.
        for (const auto& entry : fs::recursive_directory_iterator(assetRoot)) {
            if (!entry.is_regular_file())
                continue;

            const fs::path& filePath = entry.path();
            std::string extension = filePath.extension().string();

            // Normalize to lowercase
            for (char& c : extension)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (extension == ".obj")
                meshFiles.push_back(filePath);
            else if (IsTextureExtension(extension))
                textureFiles.push_back(filePath);
            else if (extension == ".mtl")
                mtlFiles.push_back(filePath);
            else if (extension == ".mtrl")
                mtrlFiles.push_back(filePath);
            else if (IsAudioExtension(extension))
                audioFiles.push_back(filePath);
            // .lua, .scene, etc. are handled elsewhere — skip silently
        }

        // Sort for deterministic load order
        std::sort(meshFiles.begin(), meshFiles.end());
        std::sort(textureFiles.begin(), textureFiles.end());
        std::sort(mtlFiles.begin(), mtlFiles.end());
        std::sort(mtrlFiles.begin(), mtrlFiles.end());
        std::sort(audioFiles.begin(), audioFiles.end());

        // Load order: Textures first, then material files, then Meshes,
        // then audio clips.
        for (const auto& path : textureFiles)
            LoadTexture(path.string());

        for (const auto& path : mtlFiles)
            LoadMTLFile(path.string());

        for (const auto& path : mtrlFiles)
            LoadMTRLFile(path.string());

        for (const auto& path : meshFiles)
            LoadMesh(path.string());

        for (const auto& path : audioFiles)
            LoadAudioClip(path.string());
    }

    void AssetManager::LoadAssetsFolder()
    {
        std::cout << "[Assets] Loading user assets from: " << m_AssetsFolderPath << "\n";
        std::cout << "[Assets] Working directory: " << fs::current_path().string() << "\n";
        LoadFolder(m_AssetsFolderPath);
        std::cout << "[Assets] User assets loaded.\n";
    }

    void AssetManager::LoadEngineAssetsFolder()
    {
        std::cout << "[Assets] Loading engine assets from: " << m_EngineAssetsFolderPath << "\n";
        LoadFolder(m_EngineAssetsFolderPath);
        std::cout << "[Assets] Engine assets loaded.\n";
    }


    // -----------------------------------------------------------------------
    // Path helpers
    // -----------------------------------------------------------------------

    std::string AssetManager::ToSceneRef(const std::string& absPath)
    {
        // Prefer engine assets prefix first (so engine assets under a sub-path of
        // the user assets folder still get the correct prefix).
        if (!m_EngineAssetsFolderPath.empty() && absPath.find(m_EngineAssetsFolderPath) == 0) {
            std::string rel = absPath.substr(m_EngineAssetsFolderPath.size());
            std::replace(rel.begin(), rel.end(), '\\', '/');
            return "engine://" + rel;
        }
        if (!m_AssetsFolderPath.empty() && absPath.find(m_AssetsFolderPath) == 0) {
            std::string rel = absPath.substr(m_AssetsFolderPath.size());
            std::replace(rel.begin(), rel.end(), '\\', '/');
            return rel;
        }
        return absPath;
    }

    std::string AssetManager::FromSceneRef(const std::string& ref)
    {
        constexpr std::string_view enginePrefix = "engine://";
        if (ref.rfind(enginePrefix, 0) == 0) {
            std::string rel = ref.substr(enginePrefix.size());
            std::replace(rel.begin(), rel.end(), '/', '\\');
            return m_EngineAssetsFolderPath + rel;
        }
        std::string result = m_AssetsFolderPath + ref;
        std::replace(result.begin(), result.end(), '/', '\\');
        return result;
    }

    // -----------------------------------------------------------------------
    // .mtrl file parser (custom engine material format)
    // -----------------------------------------------------------------------
    // Format:
    //   dt <texture_path | none>   diffuse texture
    //   c  r g b a                 color tint
    //   sc r g b                   specular color
    //   ss shininess               specular shininess
    //   it 0|1                     is transparent
    // -----------------------------------------------------------------------

    void AssetManager::LoadMTRLFile(const std::string& mtrlPath)
    {
        std::ifstream file(mtrlPath);
        if (!file.is_open()) {
            std::cout << "[Assets] Could not open .mtrl: " << mtrlPath << "\n";
            return;
        }

        // One .mtrl = one material, named after the file stem
        fs::path p(mtrlPath);
        std::string matName = p.stem().string();
        std::string matKey  = mtrlPath + "::" + matName;

        if (m_MaterialPathToID.find(matKey) != m_MaterialPathToID.end())
            return; // already loaded

        std::string diffuseTexRelPath;
        glm::vec4 color(0.9f, 0.9f, 0.9f, 1.0f);
        glm::vec3 specColor(0.5f, 0.5f, 0.5f);
        float shininess = 32.0f;
        bool transparent = false;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss(line);
            std::string key;
            ss >> key;
            if (key == "dt") {
                std::string val; ss >> val;
                if (val != "none") diffuseTexRelPath = val;
            } else if (key == "c") {
                ss >> color.r >> color.g >> color.b >> color.a;
            } else if (key == "sc") {
                ss >> specColor.r >> specColor.g >> specColor.b;
            } else if (key == "ss") {
                ss >> shininess;
            } else if (key == "it") {
                int v = 0; ss >> v; transparent = (v != 0);
            }
        }

        // Resolve texture path
        std::string resolvedTexPath;
        if (!diffuseTexRelPath.empty()) {
            // Try absolute first, then relative to engine assets, then user assets
            if (fs::exists(diffuseTexRelPath)) {
                resolvedTexPath = diffuseTexRelPath;
            } else {
                // Try relative to the .mtrl file's directory
                fs::path mtrlDir = p.parent_path();
                fs::path candidate = mtrlDir / diffuseTexRelPath;
                if (fs::exists(candidate)) {
                    resolvedTexPath = candidate.string();
                } else if (!m_EngineAssetsFolderPath.empty()) {
                    std::string engPath = m_EngineAssetsFolderPath + diffuseTexRelPath;
                    std::replace(engPath.begin(), engPath.end(), '/', '\\');
                    if (fs::exists(engPath)) resolvedTexPath = engPath;
                }
                if (resolvedTexPath.empty() && !m_AssetsFolderPath.empty()) {
                    std::string userPath = m_AssetsFolderPath + diffuseTexRelPath;
                    std::replace(userPath.begin(), userPath.end(), '/', '\\');
                    if (fs::exists(userPath)) resolvedTexPath = userPath;
                }
            }

            if (!resolvedTexPath.empty() && GetTextureID(resolvedTexPath) == INVALID_ASSET_ID)
                LoadTexture(resolvedTexPath);
        }

        // Build the Material
        auto material = std::make_shared<Material>();
        material->SetShader(Renderer::GetLitShader());
        material->SetColor(color);
        material->SetSpecularColor(specColor);
        material->SetShininess(shininess);
        material->SetTransparent(transparent);

        if (!resolvedTexPath.empty()) {
            AssetID texID = GetTextureID(resolvedTexPath);
            if (texID != INVALID_ASSET_ID)
                material->SetDiffuseTexture(GetTexture(texID));
        }

        AssetID assetID = m_NextAssetID++;

        MaterialAsset asset;
        asset.assetID           = assetID;
        asset.filePath          = matKey;
        asset.name              = matName;
        asset.colorTint         = color;
        asset.specularColor     = specColor;
        asset.specularShininess = shininess;
        asset.isTransparent     = transparent;

        if (!resolvedTexPath.empty()) {
            AssetID texID = GetTextureID(resolvedTexPath);
            if (texID != INVALID_ASSET_ID) {
                auto* texAsset = GetTextureAsset(texID);
                if (texAsset) asset.diffuseTexture = *texAsset;
            }
        }

        m_MaterialAssets[assetID] = asset;
        m_LoadedMaterials[assetID] = material;
        m_MaterialPathToID[matKey] = assetID;

        std::cout << "[MTRL->Material] '" << matName << "' -> AssetID " << assetID << "\n";
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

        // Use the full OBJ loader to get geometry + material refs
        OBJResult objResult;
        if (!OBJLoader::Load(filePath, objResult)) {
            std::cout << "Failed to parse OBJ: " << filePath << "\n";
            return;
        }

        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
        if (!mesh->Create(objResult.vertices, objResult.indices)) {
            std::cout << "Failed to create mesh: " << filePath << "\n";
            return;
        }

        AssetID assetID = m_NextAssetID++;

        MeshAsset asset;
        asset.assetID = assetID;
        asset.filePath = filePath;

        std::filesystem::path path(filePath);
        asset.name = path.stem().string();

        m_MeshAssets[assetID] = asset;
        m_LoadedMeshes[assetID] = mesh;
        m_MeshPathToID[filePath] = assetID;

        std::cout << "OBJ loaded to AssetID: " << assetID << " --- " << filePath << "\n"
            << " --- Vertices: " << objResult.vertices.size() << "\n"
            << " --- Indices: " << objResult.indices.size() << "\n";

        // Auto-import materials from the .mtl file referenced by this OBJ
        if (!objResult.mtlLibPath.empty())
            LoadMTLFile(objResult.mtlLibPath);
    }

    void AssetManager::LoadTexture(const std::string filePath)
    {
        // Avoid duplicate loads
        auto existing = m_TexturePathToID.find(filePath);
        if (existing != m_TexturePathToID.end())
            return;

        std::shared_ptr<Texture> texture = std::make_shared<Texture>();

        if (!texture->LoadFromFile(filePath)) {
            std::cout << "Failed to load texture: " << filePath << "\n";
            return;
        }

        AssetID assetID = m_NextAssetID++;

        TextureAsset asset;
        asset.assetID = assetID;
        asset.filePath = filePath;
        std::filesystem::path path(filePath);
        asset.name = path.stem().string();

        m_TextureAssets[assetID] = asset;
        m_LoadedTextures[assetID] = texture;
        m_TexturePathToID[filePath] = assetID;

        std::cout << "Texture loaded to AssetID: " << assetID << " --- " << filePath << "\n";
    }

    void AssetManager::LoadMaterial(const std::string filePath)
    {
        // .mtl files are parsed via LoadMTLFile (may contain multiple materials)
        std::filesystem::path p(filePath);
        std::string ext = p.extension().string();
        for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (ext == ".mtl") {
            LoadMTLFile(filePath);
            return;
        }

        std::cout << "Unsupported material format: " << filePath << "\n";
    }


    void AssetManager::LoadMTLFile(const std::string& mtlPath)
    {
        std::vector<MTLMaterial> mtlMaterials;
        if (!MTLLoader::Load(mtlPath, mtlMaterials))
            return;

        // For each material in the .mtl, auto-load its textures and create a Material asset
        for (const MTLMaterial& mtlMat : mtlMaterials)
        {
            // Build a virtual path key: "<mtl_path>::<material_name>"
            // This allows multiple .mtl files to have materials with the same name
            std::string matKey = mtlPath + "::" + mtlMat.name;

            // Skip if already loaded
            if (m_MaterialPathToID.find(matKey) != m_MaterialPathToID.end())
                continue;

            // Resolve and auto-load the diffuse texture if referenced
            std::string resolvedTexPath = mtlMat.diffuseMap;
            if (!resolvedTexPath.empty()) {
                // If the path from MTLLoader isn't absolute or doesn't exist,
                // try resolving relative to the user assets folder, then engine assets.
                if (!std::filesystem::exists(resolvedTexPath)) {
                    std::string assetsRelative = m_AssetsFolderPath + resolvedTexPath;
                    std::replace(assetsRelative.begin(), assetsRelative.end(), '/', '\\');
                    if (std::filesystem::exists(assetsRelative)) {
                        resolvedTexPath = assetsRelative;
                    } else if (!m_EngineAssetsFolderPath.empty()) {
                        std::string engRelative = m_EngineAssetsFolderPath + resolvedTexPath;
                        std::replace(engRelative.begin(), engRelative.end(), '/', '\\');
                        if (std::filesystem::exists(engRelative))
                            resolvedTexPath = engRelative;
                    }
                }

                // Check if already loaded under any path
                AssetID existingTex = GetTextureID(resolvedTexPath);
                if (existingTex == INVALID_ASSET_ID)
                    LoadTexture(resolvedTexPath);
            }

            // Build engine Material
            auto material = std::make_shared<Material>();
            material->SetShader(Renderer::GetLitShader());
            material->SetColor(glm::vec4(mtlMat.diffuseColor, mtlMat.opacity));
            material->SetSpecularColor(mtlMat.specularColor);
            material->SetShininess(mtlMat.shininess);
            material->SetTransparent(mtlMat.opacity < 0.99f);

            // Attach diffuse texture if loaded
            if (!resolvedTexPath.empty()) {
                AssetID texID = GetTextureID(resolvedTexPath);
                if (texID != INVALID_ASSET_ID)
                    material->SetDiffuseTexture(GetTexture(texID));
            }

            // Register as asset
            AssetID assetID = m_NextAssetID++;

            MaterialAsset asset;
            asset.assetID = assetID;
            asset.filePath = matKey;
            asset.name = mtlMat.name;
            asset.colorTint = glm::vec4(mtlMat.diffuseColor, mtlMat.opacity);
            asset.specularColor = mtlMat.specularColor;
            asset.specularShininess = mtlMat.shininess;
            asset.isTransparent = mtlMat.opacity < 0.99f;

            if (!resolvedTexPath.empty()) {
                AssetID texID = GetTextureID(resolvedTexPath);
                if (texID != INVALID_ASSET_ID) {
                    auto* texAsset = GetTextureAsset(texID);
                    if (texAsset)
                        asset.diffuseTexture = *texAsset;
                }
            }

            m_MaterialAssets[assetID] = asset;
            m_LoadedMaterials[assetID] = material;
            m_MaterialPathToID[matKey] = assetID;

            std::cout << "[MTL->Material] '" << mtlMat.name << "' -> AssetID " << assetID << "\n";
        }
    }


    void AssetManager::RekeyMaterial(const std::string& oldKey, const std::string& newKey)
    {
        auto it = m_MaterialPathToID.find(oldKey);
        if (it != m_MaterialPathToID.end()) {
            AssetID id = it->second;
            m_MaterialPathToID.erase(it);
            m_MaterialPathToID[newKey] = id;
            std::cout << "[Assets] Re-keyed material: " << oldKey << " -> " << newKey << "\n";
        }
    }

    void AssetManager::SaveAllMaterials()
    {
        // Group materials by their .mtl file path
        // Key format is "<abs_mtl_path>::<material_name>"
        std::unordered_map<std::string, std::vector<AssetID>> mtlFileToIDs;

        for (const auto& [key, id] : m_MaterialPathToID) {
            size_t sep = key.find("::");
            if (sep == std::string::npos) continue;
            std::string mtlPath = key.substr(0, sep);
            mtlFileToIDs[mtlPath].push_back(id);
        }

        for (const auto& [mtlPath, ids] : mtlFileToIDs) {
            // Skip engine assets — they are read-only built-in resources
            if (!m_EngineAssetsFolderPath.empty() && mtlPath.find(m_EngineAssetsFolderPath) == 0)
                continue;

            std::ofstream file(mtlPath);
            if (!file.is_open()) {
                std::cout << "[Assets] Failed to write: " << mtlPath << "\n";
                continue;
            }

            file << "# Wavefront Material\n";

            for (AssetID id : ids) {
                MaterialAsset* asset = GetMaterialAsset(id);
                if (!asset) continue;

                file << "\nnewmtl " << asset->name << "\n";
                file << "Kd " << asset->colorTint.r << " " << asset->colorTint.g << " " << asset->colorTint.b << "\n";
                file << "Ks " << asset->specularColor.r << " " << asset->specularColor.g << " " << asset->specularColor.b << "\n";
                file << "Ns " << asset->specularShininess << "\n";
                file << "d " << asset->colorTint.a << "\n";

                // Write texture reference if one is assigned
                if (asset->diffuseTexture.assetID != INVALID_ASSET_ID && !asset->diffuseTexture.filePath.empty()) {
                    // Try to make path relative to the .mtl file's directory
                    fs::path texAbsPath(asset->diffuseTexture.filePath);
                    fs::path mtlDir = fs::path(mtlPath).parent_path();
                    std::string texRef = asset->diffuseTexture.filePath;

                    // If both are under the assets folder, make relative to .mtl dir
                    auto relPath = fs::relative(texAbsPath, mtlDir);
                    if (!relPath.empty() && relPath.string().find("..") != 0) {
                        texRef = relPath.string();
                    }
                    std::replace(texRef.begin(), texRef.end(), '\\', '/');
                    file << "map_Kd " << texRef << "\n";
                }
            }

            file.close();
            std::cout << "[Assets] Saved: " << mtlPath << "\n";
        }
    }

    void AssetManager::LoadAudioClip(const std::string& filePath)
    {
        // Avoid duplicate loads
        if (m_AudioClipPathToID.find(filePath) != m_AudioClipPathToID.end())
            return;

        // Only register the metadata here.
        // The actual decoding happens inside AudioEngine via miniaudio.
        if (!std::filesystem::exists(filePath)) {
            std::cout << "[Assets] Audio clip not found: " << filePath << "\n";
            return;
        }

        AssetID assetID = m_NextAssetID++;

        AudioClipAsset asset;
        asset.assetID  = assetID;
        asset.filePath = filePath;
        asset.name     = std::filesystem::path(filePath).stem().string();

        m_AudioClipAssets[assetID]    = asset;
        m_AudioClipPathToID[filePath] = assetID;

        std::cout << "[Assets] Audio clip registered: AssetID " << assetID << " --- " << filePath << "\n";
    }

    void AssetManager::PrintMatsPathToID()
    {
        for (const auto& [path, id] : m_MaterialPathToID)
        {
            std::cout << "Path: " << path << " | ID: " << id << std::endl;
        }
    }
}
