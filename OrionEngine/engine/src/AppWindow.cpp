#pragma once
#include "AppWindow.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

#include <glad/glad.h>
#include "stb_image/stb_image.h"
#include <filesystem>

namespace Orion {

	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* desc)
	{
		// ORN_CORE_ERROR("GLFW Error ({0}): {1}", error, desc);
	}

	Window* Window::Create(const WindowProperties& props)
	{
		return new AppWindow(props);
	}

	AppWindow::AppWindow(const WindowProperties& props)
	{
		Init(props);
	}

	AppWindow::~AppWindow()
	{
		Shutdown();
	}

	void AppWindow::Init(const WindowProperties& props)
	{
        // use window properties to fill in AppWindow window data
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;
		
		// ORN_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized)
		{
			// TODO: glfwTerminate on system shutdown
			if (!glfwInit())
			{
				printf("Could not initialize GLFW!");
			}

			glfwSetErrorCallback(GLFWErrorCallback);

			s_GLFWInitialized = true;
		}

        // ------------------------GLFW AND GLAD INITIALIZATION------------------------
		// Tell GLFW what kind of OpenGL context we want.
		// Here we ask for OpengGL 4.6 Core Profile
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWmonitor* monitor = nullptr;
		if (props.Fullscreen) {
			monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			// Use the monitor's native resolution in fullscreen
			m_Data.Width = mode->width;
			m_Data.Height = mode->height;
		}

		m_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), monitor, nullptr);
		glfwMakeContextCurrent(m_Window);
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			printf("Failed to initialize Glad!");
		}
		glfwSetWindowUserPointer(m_Window, &m_Data);

		// Set window icon from the engine assets
		{
			std::string iconPath = "../engine/engineAssets/icons/Orion_Engine_Icon_TRANSPARENT.png";
			int w, h, channels;
			unsigned char* pixels = stbi_load(iconPath.c_str(), &w, &h, &channels, 4);
			if (pixels) {
				GLFWimage icon;
				icon.width = w;
				icon.height = h;
				icon.pixels = pixels;
				glfwSetWindowIcon(m_Window, 1, &icon);
				stbi_image_free(pixels);
			}
		}

		// Enable VSync.
		// With VSync on, buffer swaps wiat for the monitor refresh.
		// So if your display is 60 Hz, the game usually presents around 60 fps isntead 800+ fps.
		// 1 means wait for 1 vertical refresh before swapping buffers.
		glfwSwapInterval(1);

		// ------------------------SETTING GLFW CALLBACKS------------------------
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;
                
                // create a WindowResizeEvent and run its callback to alert Application
				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
                // create a WindowCloseEvent and run its callback to alert Application
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
                    // create a KeyPressedEvent and run its callback to alert Application
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
                    // create a KeyReleasedEvent and run its callback to alert Application
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
                    // create a KeyPressedEvent (indicating repeat) and run its callback to alert Application
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
                    // create a MouseBtnPressedEvent and run its callback to alert Application
					MouseBtnPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
                    // create a MouseBtnReleasedEvent and run its callback to alert Application
					MouseBtnReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                // create a MouseScrolledEvent and run its callback to alert Application
				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                // create a MouseMovedEvent and run its callback to alert Application
				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});
	}

	void AppWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void AppWindow::OnUpdate()
	{
        // poll events and swap buffers on every frame update
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}

	// void AppWindow::SetVSync(bool enabled)
	// {
	// 	if (enabled)
	// 		glfwSwapInterval(1);
	// 	else
	// 		glfwSwapInterval(0);

	// 	m_Data.VSync = enabled;
	// }

	// bool AppWindow::IsVSync() const
	// {
	// 	return m_Data.VSync;
	// }

}