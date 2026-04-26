#pragma once
#include "Layer.h"
#include "Layers/RuntimeLayer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "ECS/Scene.h"
#include <Renderer/EditorCamera.h>
#include <Renderer/Gizmo.h>

#include <string>


namespace Orion {

	class ORION_API EditorLayer : public Layer {

	public:
		EditorLayer();
		~EditorLayer();
		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate() override;
		void OnEvent(Event& event) override;

		static EntityID GetSelectedEntity() { return s_SelectedEntity; }
		static void SetSelectedEntity(EntityID entity) { s_SelectedEntity = entity; }
		static GizmoMode GetGizmoMode() { return s_GizmoMode; }
		static PlayState GetPlayState() { return s_PlayState; }
		static void SetPlayState(PlayState state) { s_PlayState = state; }
		static bool IsPlaying() { return s_PlayState == PlayState::Playing; }

		// Singleton accessor — set in constructor, valid for the lifetime of the layer.
		static EditorLayer* Get() { return s_Instance; }

		// Safe static wrappers — call via ImGuiLayer buttons, etc.
		static void RequestEnterPlay() { if (s_Instance) s_Instance->EnterPlayMode(); }
		static void RequestExitPlay()  { if (s_Instance) s_Instance->ExitPlayMode(); }

		// Create an entity in the active scene for a built-in primitive.
		// `name` is the display name ("Cube", "Sphere", …).
		// `modelFileName` is the stem of the .obj under assets/models/ (e.g. "cube").
		// Looks up the mesh + default material from AssetManager; creates and selects the entity.
		static void AddPrimitive(const std::string& name, const std::string& modelFileName);

		// Play mode
		void EnterPlayMode();
		void ExitPlayMode();

	private:
		// Attempt to start a gizmo drag. Returns true if an axis was grabbed.
		bool TryBeginGizmoDrag(float screenMouseX, float screenMouseY);
		void UpdateGizmoDrag(float screenMouseX, float screenMouseY);
		void EndGizmoDrag();

	private:
		static EntityID s_SelectedEntity;
		static EntityID s_ClipboardEntity;  // Last entity copied via Ctrl+C; source for Ctrl+V paste.
		static GizmoMode s_GizmoMode;
		static PlayState s_PlayState;
		static EditorLayer* s_Instance;

		static EditorCamera s_EditorCamera;

		// Runtime layer (owned by EditorLayer, pushed/popped from LayerStack)
		RuntimeLayer* m_RuntimeLayer = nullptr;

		// Gizmo drag state
		bool m_DraggingGizmo = false;
		GizmoAxis m_DragAxis = GizmoAxis::None;
		float m_LastDragMouseX = 0.0f;
		float m_LastDragMouseY = 0.0f;
	};
}