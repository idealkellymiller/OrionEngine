#pragma once

#include <functional>
#include "Events/Event.h"

namespace Orion
{
    // struct in format GLFW can use to create a GLFW window
    struct WindowProperties
    {
        std::string Title;
        unsigned int Width;
        unsigned int Height;

        // constructor (w/ default properties)
        WindowProperties(const std::string& title = "Orion Engine Editor",
            unsigned int width = 1920,
            unsigned int height = 1080) : Title(title), Width(width), Height(height) {}
    };

    class Window
    {
	public:
		using EventCallbackFn = std::function<void(Event&)>;

        // constructor
		virtual ~Window() {}

		virtual void OnUpdate() = 0;

        virtual unsigned int GetWidth() const = 0;
        virtual unsigned int GetHeight() const = 0;
        // used to set this window's event callback to Application's OnEvent
        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

        static Window* Create(const WindowProperties& props = WindowProperties());

    };
}