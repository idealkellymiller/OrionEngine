#pragma once
#include "Layer.h"

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
		void OnAttach();
		void OnDetach();
		void OnUpdate();
		void OnEvent(Event& event);

		static EntityID GetSelectedEntity() { return s_SelectedEntity; }
		static GizmoMode GetGizmoMode() { return s_GizmoMode; }

		void AddPrimitive(std::string primitiveFileName);

	private:
		// Attempt to start a gizmo drag. Returns true if an axis was grabbed.
		bool TryBeginGizmoDrag(float screenMouseX, float screenMouseY);
		void UpdateGizmoDrag(float screenMouseX, float screenMouseY);
		void EndGizmoDrag();

	private:
		static EntityID s_SelectedEntity;
		static GizmoMode s_GizmoMode;

		static EditorCamera s_EditorCamera;

		// Gizmo drag state
		bool m_DraggingGizmo = false;
		GizmoAxis m_DragAxis = GizmoAxis::None;
		float m_LastDragMouseX = 0.0f;
		float m_LastDragMouseY = 0.0f;
	};
}
