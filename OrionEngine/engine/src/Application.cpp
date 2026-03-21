#pragma once
#include "Application.h"
#include "Log/Log.h"

namespace Orion {

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		s_Instance = this;
		// sets this application's Window reference to Window instance
		m_Window = std::unique_ptr<Window>(Window::Create());
		// sets this application's window event callback to Application's OnEvent.
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));
	}

	Application::~Application()
	{
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnDetach();
	}

	// after receiving an event, dispatch to the layers of the app
	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		// bind WindowCloseEvent's function to be Application's OnWindowClose
		// this allows the app window to close when user clicks the X button
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));

		// go through every layer in the stack.
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();) 
		{
			// dispatch the event to the next layer
			(*--it)->OnEvent(e);
			// if the layer handled the event, don't continue propagating through stack
			if (e.Handled)
			{
				break;
			}
		}

	}

	void Application::Run()
	{
		//--------------------------MAIN APP LOOP--------------------------
		while (m_Running) {

			// update every layer in the stack in order
			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate();
			}

			// update app window
			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
}