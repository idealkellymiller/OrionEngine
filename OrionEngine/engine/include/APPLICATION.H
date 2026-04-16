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

            // Request a graceful shutdown at the end of the current frame.
            // Callable from any layer or script binding.
            void Close() { m_Running = false; }

            // Uniform time scaling. Read by runtime simulation layers to scale their dt.
            // 1.0 = normal, 0.0 = paused, 0.5 = slow-mo, 2.0 = fast-forward.
            static float GetTimeScale() { return s_TimeScale; }
            static void SetTimeScale(float scale) { s_TimeScale = (scale < 0.0f) ? 0.0f : scale; }

        protected:
            static EditorLayer* s_EditorLayer; // instance of EditorLayer

        private:
            static Application* s_Instance; // static singleton instance of Application
            static float s_TimeScale;       // uniform time-scale multiplier for simulation dt
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