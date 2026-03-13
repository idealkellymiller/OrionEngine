#pragma once

#include <GLFW/glfw3.h>
#include "Window.h"

namespace Orion
{
    class AppWindow : public Window
    {
	    public:
            virtual ~AppWindow();

            void OnUpdate() override;

            inline unsigned int GetWidth() const { return m_Data.Width; }
            inline unsigned int GetHeight() const { return m_Data.Height; }

            // used to set this window's event callback to Application's OnEvent
            inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

        private:
            GLFWwindow* m_Window;

            struct WindowData
            {
                std::string Title;
                unsigned int Width, Height;

                EventCallbackFn EventCallback;
            };

            WindowData m_Data;
    };
}