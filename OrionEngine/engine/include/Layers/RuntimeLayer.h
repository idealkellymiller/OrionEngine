#pragma once
#include "Layer.h"
#include "ECS/Scene.h"
#include "Scripting/Apollo.h"
#include "Physics/PhysicsWorld.h"

#include <memory>

namespace Orion {

	enum class PlayState {
		Stopped,
		Playing
	};

	class ORION_API RuntimeLayer : public Layer {

	public:
		RuntimeLayer();
		~RuntimeLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate() override;
		void OnEvent(Event& event) override;

		// Called by EditorLayer when entering/exiting play mode
		void BeginPlay(std::shared_ptr<Scene> editorScene);
		void EndPlay();

		// Access the runtime scene (the active copy being simulated)
		std::shared_ptr<Scene> GetRuntimeScene() const { return m_RuntimeScene; }

		// Get the saved editor scene snapshot for restoration
		std::shared_ptr<Scene> GetEditorSceneSnapshot() const { return m_EditorSceneSnapshot; }

	private:
		// Finds the active CameraComponent entity and writes its transform
		// into the Renderer's active Camera each frame.
		void ApplyRuntimeCamera();

	private:
		std::shared_ptr<Scene> m_RuntimeScene = nullptr;		 // The live scene being simulated
		std::shared_ptr<Scene> m_EditorSceneSnapshot = nullptr;  // Frozen copy of the editor scene

		float m_RuntimeTime = 0.0f;

		// Apollo scripting engine — owns the Lua VM for this play session.
		ScriptEngine m_ScriptEngine;

		// Jolt physics world — created at BeginPlay, destroyed at EndPlay.
		PhysicsWorld m_PhysicsWorld;
	};

}
