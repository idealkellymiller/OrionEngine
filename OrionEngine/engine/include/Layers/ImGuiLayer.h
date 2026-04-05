#pragma once
#include "Layer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "imgui.h"
#include "ECS/Scene.h"

namespace Orion {
	struct HierarchyNode
	{
		std::string name;
		std::vector<HierarchyNode> children;
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

	private:
		float m_Time = 0.0f;
		ImVec2 m_ViewportImageMin = { 0, 0 };
		ImVec2 m_ViewportImageMax = { 0, 0 };
		static EntityID s_HoveredEntity;

		static bool s_ViewportHovered;
		static bool s_ViewportFocused;

		// --------ImGui Module Definitions-----------
		void ShowMainMenuBar();
		void ShowTransformComponent();
		void ShowMeshColliderComponent();
		void ShowInspectorModule();
		void ShowViewportModule();
		void DrawHierarchyNode(HierarchyNode& node);
		void ShowHierarchyModule();
		void DrawDirectoryTree();
		void DrawDirectorySearch();
		void DrawDirectory();
		void ShowConsoleTraceOutput(const char* source, const char* message);
		void ShowConsoleWarningOutput(const char* source, const char* message);
		void ShowConsoleErrorOutput(const char* source, const char* message);
		void ShowConsoleModule();
		void ShowControlsModule();

		// temporary
		void AddPrimitive();

	};
}