#pragma once

#include "EngineCore.h"

#include "Events/Event.h"
#include "Window.h"
#include "Layers/LayerStack.h"
#include "Layers/EditorLayer.h"
#include "Events/ApplicationEvent.h"
#include "Core/ProjectSettings.h"

#include <memory>
#include <vector>
#include <functional>
#include <iostream>

namespace Orion
{
    class ORION_API Application
    {
        public:
            Application(const WindowProperties& props = WindowProperties());
            virtual ~Application();

            inline static Application& Get() { return *s_Instance; }
            inline static EditorLayer& GetEditorLayer() { return *s_EditorLayer; }
            inline Window& GetWindow() { return *m_Window; }

            void PushLayer(Layer* layer);
            void PushOverlay(Layer* layer);
            void PopLayer(Layer* layer);
            void PopOverlay(Layer* layer);

            // Queue a layer operation to run safely between frames.
            // Use this when pushing/popping layers from inside OnUpdate or OnEvent.
            void QueueLayerOp(std::function<void()> op);

            void Run(int argc = 0, char** argv = nullptr);

            // Run just the main loop (no asset/scene init). For standalone game runtime.
            void RunLoop();

            // all Event object EventCallbacks are set to THIS
            void OnEvent(Event& e);

            void SetLocalization(Language lang);

        protected:
            static EditorLayer* s_EditorLayer; // instance of EditorLayer

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