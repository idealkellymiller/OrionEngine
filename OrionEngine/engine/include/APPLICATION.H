#pragma once

#include "EngineCore.h"

#include "Events/Event.h"
#include "Window.h"
#include "Layers/LayerStack.h"
#include "Events/ApplicationEvent.h"

#include <memory>

namespace Orion
{
    class ORION_API Application
    {
        public:
            Application();
            virtual ~Application();
            void PushLayer(Layer* layer);
            void PushOverlay(Layer* layer);

            void Run();

            // all Event object EventCallbacks are set to THIS
            void OnEvent(Event& e);


        private:
            bool OnWindowClose(WindowCloseEvent& e);
            std::unique_ptr<Window> m_Window; // points to main app window
            bool m_Running = true;

            LayerStack m_LayerStack;
    };
    
    // defined in editor client
    Application* CreateApplication();
}