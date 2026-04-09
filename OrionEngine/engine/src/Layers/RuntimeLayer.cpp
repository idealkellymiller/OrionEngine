#include "EngineCore.h"
#include "Layers/RuntimeLayer.h"
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

		// Compute delta time for this frame.
		static float lastTime = (float)glfwGetTime();
		float now = (float)glfwGetTime();
		float dt = now - lastTime;
		lastTime = now;

		m_RuntimeTime += dt;

		// --- Apollo scripting: tick all entity scripts ---
		if (m_ScriptEngine.IsInitialized())
			m_ScriptEngine.OnUpdate(dt);

		// --- Step physics simulation ---
		if (m_PhysicsWorld.IsInitialized())
			m_PhysicsWorld.Step(dt);

		// --- Drive the renderer camera from the active CameraComponent ---
		ApplyRuntimeCamera();
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
