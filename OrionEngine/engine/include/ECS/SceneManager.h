#pragma once
#include "EngineCore.h"
#include <memory>
#include <string>



namespace Orion {

	class Scene;


	class ORION_API SceneManager {
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

		// Path the active scene was loaded from (empty if unsaved)
		static const std::string& GetActiveScenePath() { return s_ActiveScenePath; }

	private:
		static std::shared_ptr<Scene> s_ActiveScene;
		static std::string s_ActiveScenePath;
	};
}