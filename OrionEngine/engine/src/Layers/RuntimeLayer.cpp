#include "EngineCore.h"
#include "Layers/RuntimeLayer.h"
#include "Layers/EditorLayer.h"
#include "Application.h"
#include "Core/ProjectSettings.h"
#include "ECS/SceneManager.h"
#include "Assets/AssetManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>
#include <GLFW/glfw3.h>


namespace Orion {

	RuntimeLayer::RuntimeLayer() : Layer("RuntimeLayer")
	{
	}

	RuntimeLayer::~RuntimeLayer()
	{
	}

	void RuntimeLayer::OnAttach()
	{
	}

	void RuntimeLayer::OnDetach()
	{
		// If still playing when detached, clean up
		if (m_RuntimeScene)
			EndPlay();
	}

	void RuntimeLayer::OnUpdate()
	{
		if (!m_RuntimeScene)
			return;

		// Compute delta time for this frame, then apply the global time-scale
		// (set via Application.SetTimeScale from Lua or engine code). Using an
		// unscaled-then-scaled split means scripts and physics agree on dt.
		static float lastTime = (float)glfwGetTime();
		float now = (float)glfwGetTime();
		float rawDt = now - lastTime;
		lastTime = now;

		float dt = rawDt * Application::GetTimeScale();

		m_RuntimeTime += dt;

		// --- Apollo scripting: tick all entity scripts ---
		if (m_ScriptEngine.IsInitialized()) {
			// Pick up any .lua files edited on disk since the last frame.
			// Reloaded scripts re-run OnStart() internally, then OnUpdate ticks them normally.
			m_ScriptEngine.CheckHotReload();
			m_ScriptEngine.OnUpdate(dt);
		}

		// --- Step physics simulation ---
		if (m_PhysicsWorld.IsInitialized())
			m_PhysicsWorld.Step(dt);

		// --- Drive the renderer camera from the active CameraComponent ---
		ApplyRuntimeCamera();

		// --- Handle Scene.Load / Scene.Reload requests from scripts ---
		// A script may have called Scene.Load("next.scene") during OnUpdate above.
		// We can't swap scenes mid-update, so defer to the next frame via the
		// Application's layer-op queue (same mechanism EnterPlayMode uses).
		if (m_ScriptEngine.IsInitialized()) {
			const std::string& pending = m_ScriptEngine.GetPendingSceneLoad();
			if (!pending.empty()) {
				std::string path = pending;  // copy; ClearPending invalidates the ref
				m_ScriptEngine.ClearPendingSceneLoad();

				// Resolve "__RELOAD__" to the currently active scene's path.
				bool reload = (path == "__RELOAD__");
				if (reload)
					path = SceneManager::GetActiveScenePath();

				// If path is still empty we have nothing sensible to load — skip.
				if (!path.empty()) {
					Application::Get().QueueLayerOp([this, path]() {
						// Resolve path: if it looks relative (no drive letter, no leading
						// slash) prefix it with the assets folder for convenience.
						const std::string assetsPath = AssetManager::GetAssetsFolderPath();
						std::string fullPath = path;
						if (fullPath.find(':') == std::string::npos &&
						    (fullPath.empty() || (fullPath[0] != '/' && fullPath[0] != '\\')))
						{
							fullPath = assetsPath + fullPath;
						}

						// Tear down physics + scripting against the old runtime scene.
						// We deliberately do NOT call EndPlay() — that would discard the
						// original editor snapshot, so exiting play mode would restore
						// into the newly-loaded scene instead of the user's edit target.
						if (m_PhysicsWorld.IsInitialized())
							m_PhysicsWorld.Shutdown();
						if (m_ScriptEngine.IsInitialized())
							m_ScriptEngine.Shutdown();

						// Load the requested scene into SceneManager.
						SceneManager::LoadScene(fullPath);
						auto loaded = SceneManager::GetActiveScene();
						if (!loaded) {
							std::cout << "[RuntimeLayer] Scene.Load failed: " << fullPath << "\n";
							return;
						}

						// Swap in a fresh runtime copy of the loaded scene.
						m_RuntimeScene = loaded->Copy();
						SceneManager::SetActiveScene(m_RuntimeScene);
						m_RuntimeTime = 0.0f;

						// Re-init scripts and physics against the new runtime scene.
						// Note: m_EditorSceneSnapshot is untouched, so ExitPlayMode later
						// still restores the original pre-play editor scene.
						m_ScriptEngine.Init(m_RuntimeScene, assetsPath, &m_PhysicsWorld);
						m_PhysicsWorld.Init(m_RuntimeScene);
						m_PhysicsWorld.SetCollisionCallback(
							[this](EntityID a, EntityID b, bool isTrigger) {
								if (m_ScriptEngine.IsInitialized())
									m_ScriptEngine.OnCollision(a, b, isTrigger);
							}
						);
						m_ScriptEngine.OnStart();

						std::cout << "[RuntimeLayer] Loaded scene during play: " << fullPath << "\n";
					});
				}
			}
		}
	}

	void RuntimeLayer::OnEvent(Event& event)
	{
		// During play mode, runtime-specific input can be handled here.
		// For now, events pass through to other layers.
		// (Scripts read input via Input.IsKeyDown polling, not events.)
	}

	void RuntimeLayer::BeginPlay(std::shared_ptr<Scene> editorScene)
	{
		// 1. Snapshot the editor scene so we can restore it later
		m_EditorSceneSnapshot = editorScene->Copy();

		// 2. Create a separate runtime copy to simulate on
		m_RuntimeScene = editorScene->Copy();

		// 3. Swap the active scene to the runtime copy
		SceneManager::SetActiveScene(m_RuntimeScene);

		m_RuntimeTime = 0.0f;

		// 4. Initialize Apollo scripting — loads all ScriptComponents
		//    Pass the PhysicsWorld pointer so Lua scripts can call Physics.AddForce(), etc.
		m_ScriptEngine.Init(m_RuntimeScene, AssetManager::GetAssetsFolderPath(), &m_PhysicsWorld);

		// 5. Initialize Jolt physics world from the runtime scene
		m_PhysicsWorld.Init(m_RuntimeScene);

		// 6. Set up collision callback — forwards to script OnCollision(otherID, isTrigger)
		m_PhysicsWorld.SetCollisionCallback(
			[this](EntityID a, EntityID b, bool isTrigger) {
				if (m_ScriptEngine.IsInitialized()) {
					m_ScriptEngine.OnCollision(a, b, isTrigger);
				}
			}
		);

		// 7. Call OnStart() on all loaded scripts
		m_ScriptEngine.OnStart();

		std::cout << "[RuntimeLayer] Play mode started.\n";
	}

	void RuntimeLayer::BeginPlayStandalone(const std::string& scenePath, const std::string& assetsFolderPath)
	{
		// Mark as playing so the Renderer skips editor-only passes (grid, gizmos, picking)
		EditorLayer::SetPlayState(PlayState::Playing);

		// 1. Load assets
		AssetManager::SetAssetsFolderPath(assetsFolderPath);
		AssetManager::LoadAssetsFolder();

		// 2. Load project settings (background, lighting, etc.)
		ProjectSettings::Get().Load(assetsFolderPath + "project.settings");

		// 3. Load the scene from file
		SceneManager::LoadScene(scenePath);
		m_RuntimeScene = SceneManager::GetActiveScene();

		if (!m_RuntimeScene) {
			std::cout << "[RuntimeLayer] ERROR: Failed to load scene: " << scenePath << "\n";
			return;
		}

		m_RuntimeTime = 0.0f;

		// 3. Initialize scripting
		m_ScriptEngine.Init(m_RuntimeScene, assetsFolderPath, &m_PhysicsWorld);

		// 4. Initialize physics
		m_PhysicsWorld.Init(m_RuntimeScene);

		// 5. Collision callback
		m_PhysicsWorld.SetCollisionCallback(
			[this](EntityID a, EntityID b, bool isTrigger) {
				if (m_ScriptEngine.IsInitialized())
					m_ScriptEngine.OnCollision(a, b, isTrigger);
			}
		);

		// 6. Call OnStart() on all scripts
		m_ScriptEngine.OnStart();

		std::cout << "[RuntimeLayer] Standalone game started: " << scenePath << "\n";
	}

	void RuntimeLayer::EndPlay()
	{
		// Shut down physics before restoring the scene.
		if (m_PhysicsWorld.IsInitialized())
			m_PhysicsWorld.Shutdown();

		// Shut down Apollo before restoring the scene.
		if (m_ScriptEngine.IsInitialized())
			m_ScriptEngine.Shutdown();

		if (!m_EditorSceneSnapshot)
		{
			std::cout << "[RuntimeLayer] Warning: No editor snapshot to restore.\n";
			return;
		}

		// Restore the editor scene from snapshot
		SceneManager::SetActiveScene(m_EditorSceneSnapshot);

		// Clean up
		m_RuntimeScene.reset();
		m_EditorSceneSnapshot.reset();
		m_RuntimeTime = 0.0f;

		std::cout << "[RuntimeLayer] Play mode stopped. Scene restored.\n";
	}

	void RuntimeLayer::ApplyRuntimeCamera()
	{
		if (!m_RuntimeScene)
			return;

		Camera* rendererCam = Renderer::GetActiveCamera();
		if (!rendererCam)
			return;

		// Find the first entity with an active CameraComponent
		for (EntityID entity : m_RuntimeScene->GetEntities())
		{
			CameraComponent* camComp = m_RuntimeScene->GetCameraComponent(entity);
			if (!camComp || !camComp->isActive)
				continue;

			TransformComponent* tc = m_RuntimeScene->GetTransformComponent(entity);
			if (!tc)
				continue;

			// --- Build camera position from transform ---
			glm::vec3 camPos = tc->position;

			// --- Build forward direction from transform rotation ---
			// Default camera looks down -Z (OpenGL convention).
			// Apply the same X->Y->Z rotation order as RenderScene.
			glm::vec3 forward(0.0f, 0.0f, -1.0f);
			glm::vec3 up(0.0f, 1.0f, 0.0f);

			// Build rotation matrix from Euler angles
			glm::mat4 rot(1.0f);
			if (tc->rotation.x != 0.0f)
				rot = glm::rotate(rot, tc->rotation.x, glm::vec3(1, 0, 0));
			if (tc->rotation.y != 0.0f)
				rot = glm::rotate(rot, tc->rotation.y, glm::vec3(0, 1, 0));
			if (tc->rotation.z != 0.0f)
				rot = glm::rotate(rot, tc->rotation.z, glm::vec3(0, 0, 1));

			forward = glm::vec3(rot * glm::vec4(forward, 0.0f));
			up = glm::vec3(rot * glm::vec4(up, 0.0f));

			glm::vec3 target = camPos + forward;

			// Write to the renderer's active camera
			rendererCam->SetPosition(camPos);
			rendererCam->SetTarget(target);
			rendererCam->SetUp(up);

			// Projection — use viewport aspect ratio from the renderer
			float aspectRatio = rendererCam->GetAspectRatio();
			rendererCam->SetPerspective(camComp->fovDegrees, aspectRatio, camComp->nearPlane, camComp->farPlane);

			return; // Use only the first active camera
		}
	}

}
