#pragma once
#include "Layer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "imgui.h"
#include "ECS/Scene.h"

#include <unordered_map>

namespace Orion {

	// Component types the inspector knows how to display.
	// Order in this enum is the default display order for new entities.
	enum class ComponentType {
		EntityData,   // Name, enabled flag       — REQUIRED, cannot be removed
		Transform,    // Position, rotation, scale — REQUIRED, cannot be removed
		Mesh,         // Mesh asset reference
		Material,     // Material asset reference
		Camera,       // Camera settings (FOV, near/far, active flag)
		Script,       // Lua script path
		Rigidbody,    // Physics body (dynamic/kinematic/static, mass, damping)
		Collider      // Physics collider (box/sphere, trigger, offset)
	};

	class ORION_API ImGuiLayer : public Layer {

	public:
		ImGuiLayer();
		~ImGuiLayer();
		void OnAttach();
		void OnDetach();
		void OnUpdate();
		void OnEvent(Event& event);

		static EntityID GetHoveredEntity() { return s_HoveredEntity; }

		static bool GetViewportHovered() { return s_ViewportHovered; }
		static bool GetViewportFocused() { return s_ViewportFocused; }

		static ImVec2 GetViewportMin() { return s_ViewportImageMin; }
		static ImVec2 GetViewportMax() { return s_ViewportImageMax; }

	private:
		float m_Time = 0.0f;
		static ImVec2 s_ViewportImageMin;
		static ImVec2 s_ViewportImageMax;
		static EntityID s_HoveredEntity;

		static bool s_ViewportHovered;
		static bool s_ViewportFocused;

		// True while a mouse button is held that started in the viewport.
		// Keeps events flowing to EditorLayer even if the cursor leaves the viewport mid-drag.
		static bool s_ViewportDragging;

		// --- Inspector state ---
		// Tracks which entity the inspector is showing so we rebuild on selection change.
		EntityID m_InspectorEntity = INVALID_ENTITY;

		// Per-entity component display order (survives reselection within a session).
		std::unordered_map<EntityID, std::vector<ComponentType>> m_ComponentOrder;

		// Builds or refreshes the display-order list for the given entity.
		void RebuildComponentOrder(EntityID entity, Scene& scene);

		// Returns true if this component type is required and cannot be removed.
		static bool IsRequiredComponent(ComponentType type);

		// Draws one component's collapsing header + fields + reorder/remove buttons.
		// Returns true if the component was removed this frame.
		bool DrawComponent(ComponentType type, int index, EntityID entity, Scene& scene);

		// Individual component field drawers
		void DrawEntityDataFields(EntityID entity, Scene& scene);
		void DrawTransformFields(EntityID entity, Scene& scene);
		void DrawMeshFields(EntityID entity, Scene& scene);
		void DrawMaterialFields(EntityID entity, Scene& scene);
		void DrawCameraFields(EntityID entity, Scene& scene);
		void DrawScriptFields(EntityID entity, Scene& scene);
		void DrawRigidbodyFields(EntityID entity, Scene& scene);
		void DrawColliderFields(EntityID entity, Scene& scene);

		// "Add Component" popup
		void DrawAddComponentPopup(EntityID entity, Scene& scene);

		// --------ImGui Module Definitions-----------
		void ShowMainMenuBar();
		void ShowInspectorModule();
		void ShowViewportModule();
		void ShowHierarchyModule();
		void ShowProjectSettingsWindow();
		void DrawEntityNode(EntityID entity, Scene& scene);
		void DrawDirectoryTree();
		void DrawDirectorySearch();
		void DrawDirectory();
		void ShowConsoleTraceOutput(const char* source, const char* message);
		void ShowConsoleWarningOutput(const char* source, const char* message);
		void ShowConsoleErrorOutput(const char* source, const char* message);
		void ShowConsoleModule();
		void ShowControlsModule();

		// Drag-and-drop reparenting state
		EntityID m_DragSourceEntity = INVALID_ENTITY;

	};
}
