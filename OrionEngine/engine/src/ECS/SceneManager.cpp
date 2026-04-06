// Owns current scene and loads/saves scenes

#include "ECS/SceneManager.h"
#include "ECS/Scene.h"
#include "Assets/AssetManager.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <filesystem>
#include <algorithm>	// for std::replace

// 'vcpkg install nlohmann-json' in terminal
// OR CMAKE:
// find_package(nlohmann_json CONFIG REQUIRED)
// target_link_libraries(YourTarget PRIVATE nlohmann_json::nlohmann_json)
#include <nlohmann/json.hpp>
using json = nlohmann::json;



namespace Orion {

	std::shared_ptr<Scene> SceneManager::s_ActiveScene = nullptr;
	std::string SceneManager::s_ActiveScenePath = "";

	void SceneManager::Init()
	{
		// Start with an empty scene by default
		s_ActiveScene = std::make_shared<Scene>();
	}

	void SceneManager::Shutdown()
	{
		s_ActiveScene.reset();
	}

	void SceneManager::NewScene()
	{
		s_ActiveScene = std::make_shared<Scene>();
	}

	// Deserialize json into ecs
	bool SceneManager::LoadScene(const std::string& filePath)
	{
		std::ifstream file(filePath);
		if (!file.is_open())
		{
			std::cout << "Failed to open scene file: " << filePath << "\n";
			return false;
		}

		json j;
		try {
			file >> j;
		}
		catch (const std::exception& e) {
			std::cout << "Failed to parse scene JSON: " << e.what() << "\n";
			return false;
		}

		if (!j.contains("entities") || !j["entities"].is_array())
		{
			std::cout << "Scene file missing valid 'entities' array.\n";
			return false;
		}


		std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

		for (const auto& entityJson : j["entities"]) {
			EntityID entity = INVALID_ENTITY;

			if (entityJson.contains("id"))
				entity = newScene->CreateEntityWithID(entityJson["id"].get<EntityID>());
			else
				entity = newScene->CreateEntity();


			// --- Entity Data ---
			if (entityJson.contains("name")) {
				EntityDataComponent entityDataComp;
				entityDataComp.name = entityJson["name"].get<std::string>();
				newScene->AddEntityDataComponent(entity, entityDataComp);
			}

			// --- Transform ---
			if (entityJson.contains("transform")) {
				const auto& t = entityJson["transform"];

				TransformComponent transform;

				if (t.contains("position") && t["position"].is_array() && t["position"].size() == 3) {
					transform.position.x = t["position"][0].get<float>();
					transform.position.y = t["position"][1].get<float>();
					transform.position.z = t["position"][2].get<float>();
				}

				if (t.contains("rotation") && t["rotation"].is_array() && t["rotation"].size() == 3)
				{
					transform.rotation.x = t["rotation"][0].get<float>();
					transform.rotation.y = t["rotation"][1].get<float>();
					transform.rotation.z = t["rotation"][2].get<float>();
				}

				if (t.contains("scale") && t["scale"].is_array() && t["scale"].size() == 3)
				{
					transform.scale.x = t["scale"][0].get<float>();
					transform.scale.y = t["scale"][1].get<float>();
					transform.scale.z = t["scale"][2].get<float>();
				}

				newScene->AddTransformComponent(entity, transform);
			}

			// --- Mesh ---
			if (entityJson.contains("mesh") && entityJson["mesh"].contains("path")) {
				std::string meshPath = AssetManager::GetAssetsFolderPath() + entityJson["mesh"]["path"].get<std::string>();
				std::replace(meshPath.begin(), meshPath.end(), '/', '\\');

				AssetID meshID = AssetManager::GetMeshID(meshPath);
				if (meshID == INVALID_ASSET_ID) {
					AssetManager::LoadMesh(meshPath);
					meshID = AssetManager::GetMeshID(meshPath);
				}

				if (meshID != INVALID_ASSET_ID) {
					MeshComponent meshComp;
					meshComp.mesh = meshID;
					newScene->AddMeshComponent(entity, meshComp);
				}
				else {
					std::cout << "Warning: failed to resolve mesh: " << meshPath << "\n";
				}
			}

			// --- Material ---
			if (entityJson.contains("material") && entityJson["material"].contains("path")) {
				std::string materialPath = AssetManager::GetAssetsFolderPath() + entityJson["material"]["path"].get<std::string>();
				// Change the json '/' to '\' cause that's what the asset loader uses, but json doesn't like it
				std::replace(materialPath.begin(), materialPath.end(), '/', '\\');

				AssetID materialID = AssetManager::GetMaterialID(materialPath);
				if (materialID == INVALID_ASSET_ID) {
					AssetManager::LoadMaterial(materialPath);
					materialID = AssetManager::GetMaterialID(materialPath);
				}

				if (materialID != INVALID_ASSET_ID) {
					MaterialComponent materialComp;
					materialComp.material = materialID;
					newScene->AddMaterialComponent(entity, materialComp);
				}
				else {
					std::cout << "Warning: failed to resolve material: " << materialPath << "\n";
				}
			}

			// --- Camera ---
			if (entityJson.contains("camera")) {
				const auto& c = entityJson["camera"];

				CameraComponent cam;
				if (c.contains("fov"))       cam.fovDegrees = c["fov"].get<float>();
				if (c.contains("near"))      cam.nearPlane  = c["near"].get<float>();
				if (c.contains("far"))       cam.farPlane   = c["far"].get<float>();
				if (c.contains("isActive"))  cam.isActive   = c["isActive"].get<bool>();

				newScene->AddCameraComponent(entity, cam);
			}

			// --- Script ---
			if (entityJson.contains("script") && entityJson["script"].contains("path")) {
				ScriptComponent sc;
				sc.scriptPath = entityJson["script"]["path"].get<std::string>();
				newScene->AddScriptComponent(entity, sc);
			}
		}

		// --- Second pass: resolve parent-child relationships ---
		// All entities must exist before we can link them.
		// (All entities must exist before we can link them.)
		for (const auto& entityJson : j["entities"]) {
			if (entityJson.contains("id") && entityJson.contains("parent")) {
				EntityID child = entityJson["id"].get<EntityID>();
				EntityID parent = entityJson["parent"].get<EntityID>();
				if (newScene->IsValidEntity(child) && newScene->IsValidEntity(parent))
					newScene->SetParent(child, parent);
			}
		}

		s_ActiveScene = newScene;
		s_ActiveScenePath = filePath;
		std::cout << "Successfully loaded scene: " << filePath << "\n";
		return true;
	}

	bool SceneManager::SaveScene(const std::string& filePath)
	{
		if (!s_ActiveScene) {
			std::cout << "No active scene to save.\n";
			return false;
		}

		json j;
		j["scene"]["name"] = "Saved Scene";
		j["scene"]["version"] = 1;
		j["entities"] = json::array();

		std::string assetsPath = AssetManager::GetAssetsFolderPath();

		for (EntityID entity : s_ActiveScene->GetEntities()) {
			json entityJson;
			entityJson["id"] = entity;

			// --- Entity Data ---
			EntityDataComponent* edc = s_ActiveScene->GetEntityDataComponent(entity);
			if (edc)
				entityJson["name"] = edc->name;

			// --- Parent ---
			EntityID parent = s_ActiveScene->GetParent(entity);
			if (parent != INVALID_ENTITY)
				entityJson["parent"] = parent;

			// --- Transform ---
			TransformComponent* tc = s_ActiveScene->GetTransformComponent(entity);
			if (tc) {
				entityJson["transform"]["position"] = { tc->position.x, tc->position.y, tc->position.z };
				entityJson["transform"]["rotation"] = { tc->rotation.x, tc->rotation.y, tc->rotation.z };
				entityJson["transform"]["scale"]    = { tc->scale.x, tc->scale.y, tc->scale.z };
			}

			// --- Mesh ---
			MeshComponent* mc = s_ActiveScene->GetMeshComponent(entity);
			if (mc && mc->mesh != INVALID_ASSET_ID) {
				std::string meshPath = AssetManager::GetMeshPath(mc->mesh);
				// Convert to relative path and forward slashes for JSON
				if (meshPath.find(assetsPath) == 0)
					meshPath = meshPath.substr(assetsPath.size());
				std::replace(meshPath.begin(), meshPath.end(), '\\', '/');
				entityJson["mesh"]["path"] = meshPath;
			}

			// --- Material ---
			MaterialComponent* matc = s_ActiveScene->GetMaterialComponent(entity);
			if (matc && matc->material != INVALID_ASSET_ID) {
				std::string matPath = AssetManager::GetMaterialPath(matc->material);
				if (matPath.find(assetsPath) == 0)
					matPath = matPath.substr(assetsPath.size());
				std::replace(matPath.begin(), matPath.end(), '\\', '/');
				entityJson["material"]["path"] = matPath;
			}

			// --- Camera ---
			CameraComponent* cam = s_ActiveScene->GetCameraComponent(entity);
			if (cam) {
				entityJson["camera"]["fov"]      = cam->fovDegrees;
				entityJson["camera"]["near"]     = cam->nearPlane;
				entityJson["camera"]["far"]      = cam->farPlane;
				entityJson["camera"]["isActive"] = cam->isActive;
			}

			// --- Script ---
			ScriptComponent* sc = s_ActiveScene->GetScriptComponent(entity);
			if (sc && !sc->scriptPath.empty()) {
				entityJson["script"]["path"] = sc->scriptPath;
			}

			j["entities"].push_back(entityJson);
		}

		std::ofstream file(filePath);
		if (!file.is_open()) {
			std::cout << "Failed to open file for saving: " << filePath << "\n";
			return false;
		}

		file << j.dump(2);
		std::cout << "Successfully saved scene: " << filePath << "\n";
		return true;
	}
}