#include "EngineCore.h"
#include "Layers/RuntimeLayer.h"
#include "ECS/SceneManager.h"

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

		// Accumulate runtime clock
		static float lastTime = 0.0f;
		float now = (float)glfwGetTime();
		float dt = now - lastTime;
		lastTime = now;

		m_RuntimeTime += dt;

		// -------------------------------------------------------
		// RUNTIME GAME LOGIC GOES HERE
		// This is where scripting / gameplay systems will tick.
		// For now this is a placeholder — future work:
		//   - Tick script components (Lua, C#, native, etc.)
		//   - Tick physics
		//   - Tick audio
		//   - Tick animation
		// -------------------------------------------------------
	}

	void RuntimeLayer::OnEvent(Event& event)
	{
		// During play mode, runtime-specific input can be handled here.
		// For now, events pass through to other layers.
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

		std::cout << "[RuntimeLayer] Play mode started.\n";
	}

	void RuntimeLayer::EndPlay()
	{
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

}
