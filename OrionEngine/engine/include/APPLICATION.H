#pragma once

#include "EngineCore.h"

#include "Events/Event.h"
#include "Window.h"
#include "Layers/LayerStack.h"
#include "Events/ApplicationEvent.h"

#include <memory>
#include <vector>
#include <functional>
#include <iostream>

namespace Orion
{
    class ORION_API Application
    {
        public:
            Application();
            virtual ~Application();

            inline static Application& Get() { return *s_Instance; }
            inline Window& GetWindow() { return *m_Window; }

            void PushLayer(Layer* layer);
            void PushOverlay(Layer* layer);
            void PopLayer(Layer* layer);
            void PopOverlay(Layer* layer);

            // Queue a layer operation to run safely between frames.
            // Use this when pushing/popping layers from inside OnUpdate or OnEvent.
            void QueueLayerOp(std::function<void()> op);

            void Run();

            // all Event object EventCallbacks are set to THIS
            void OnEvent(Event& e);


        private:
            static Application* s_Instance; // static singleton instance of Application
            bool OnWindowClose(WindowCloseEvent& e);
            void ProcessPendingLayerOps();

            std::unique_ptr<Window> m_Window; // points to main app window
            bool m_Running = true;

            LayerStack m_LayerStack;
            std::vector<std::function<void()>> m_PendingLayerOps;
    };
    
    // defined in editor client
    Application* CreateApplication();
}