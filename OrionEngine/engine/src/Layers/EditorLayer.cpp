#include "EngineCore.h"
#include "Layers/EditorLayer.h"
#include "Application.h"

#include "Renderer/Renderer.h"
#include "imgui.h"

#include <iostream>


namespace Orion {


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

		
	}

	void EditorLayer::OnEvent(Event& event)
	{

	}

}