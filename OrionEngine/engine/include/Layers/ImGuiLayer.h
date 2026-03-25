#pragma once
#include "Layer.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

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

	private:
		float m_Time = 0.0f;

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