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
		static GizmoMode GetGizmoMode() { return s_GizmoMode; }
		static PlayState GetPlayState() { return s_PlayState; }
		static bool IsPlaying() { return s_PlayState == PlayState::Playing; }

		void AddPrimitive(std::string primitiveFileName);

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
		static GizmoMode s_GizmoMode;
		static PlayState s_PlayState;

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
