#include "EngineCore.h"
#include "Layers/EditorLayer.h"
#include "Layers/ImGuiLayer.h"
#include "Application.h"

#include "Renderer/Renderer.h"
#include "imgui.h"

#include <iostream>


namespace Orion {

	// Initialize static members
	EntityID EditorLayer::s_SelectedEntity = INVALID_ENTITY;

	EditorLayer::EditorLayer() : Layer("EditorLayer")
	{

	}

	EditorLayer::~EditorLayer()
	{

	}

	void EditorLayer::OnAttach()
	{

	}

	void EditorLayer::OnDetach()
	{
		Renderer::Shutdown();
	}

	void EditorLayer::OnUpdate()
	{
		Renderer::Render();

		// TODO: On click - select entity under mouse.
		s_SelectedEntity = ImGuiLayer::GetHoveredEntity();
		std::cout << s_SelectedEntity << "\n";


	}

	void EditorLayer::OnEvent(Event& event)
	{

	}

}