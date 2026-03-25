#pragma once
#include <memory>
#include <string>


class Scene;


class SceneManager {
public:
	static void Init();
	static void Shutdown();

	// Scene access
	static std::shared_ptr<Scene> GetActiveScene() { return s_ActiveScene; }
	static void SetActiveScene(std::shared_ptr<Scene> scene) { s_ActiveScene = scene; }

	// Scene loading
	static bool LoadScene(const std::string& filePath);
	static bool SaveScene(const std::string& filePath);

	static void NewScene();

private:
	static std::shared_ptr<Scene> s_ActiveScene;
};