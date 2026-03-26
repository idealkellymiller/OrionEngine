#include "EngineCore.h"
#include "Layers/ImGuiLayer.h"
#include "Application.h"
#include "Renderer/Renderer.h"

#include "imgui.h"
// #include "Platform/OpenGL/ImGuiOpenGLRenderer.h"
#include "../external/ImGUI/backends/imgui_impl_glfw.h"
#include "../external/ImGUI/backends/imgui_impl_opengl3.h"

// TODO: remove glfw include here
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <format>
#include <iostream>

namespace Orion {

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::OnAttach()
	{
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		// io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		// io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;


		Application& app = Application::Get();
		ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow()), true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnUpdate()
	{
		// --------------- Call Renderer for this frame and render to the ImGui viewport ------------------
		
		
		// -------------------------------- ImGui ------------------------------------
		
		
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

		float time = (float)glfwGetTime();
		io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
		m_Time = time;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport();

		static bool show = true;
		// ImGui::ShowDemoWindow(&show);
		ShowMainMenuBar();
		ShowInspectorModule();
		ShowViewportModule();
		ShowHierarchyModule();
		ShowConsoleModule();
		ShowControlsModule();

		// Tell ImGui to finalize all UI draw data
		ImGui::Render();



		Renderer::BeginFrame();

		// Update camera projection if window size changes
		float aspectRatio = (float)Renderer::GetViewportFramebuffer()->GetWidth() / (float)Renderer::GetViewportFramebuffer()->GetHeight();
		Renderer::GetActiveCamera()->SetPerspective(45.0f, aspectRatio, 0.1f, 100.0f);

		Renderer::Render();

		// Object Picking:
		// TODO: mousePos is being overwritten so this bool will always be false. Get mouse input through event system
		ImVec2 mousePos = ImGui::GetMousePos();
		bool mouseInsideViewportImage =
			mousePos.x >= m_ViewportImageMin.x &&
			mousePos.x < m_ViewportImageMax.x &&
			mousePos.y >= m_ViewportImageMin.y &&
			mousePos.y < m_ViewportImageMax.y;
		if (mouseInsideViewportImage) {
			int localMouseX = static_cast<int>(mousePos.x - m_ViewportImageMin.x);
			int localMouseY = static_cast<int>(mousePos.y - m_ViewportImageMin.y);
			EntityID hovered = Renderer::PickEntity(localMouseX, localMouseY);
			std::cout << "Hovered entity: " << hovered << " at (" << localMouseX << "," << localMouseY << ")\n";
		}
		else {
			// std::cout << "Hovered entity: 000\n";
		}

		Renderer::EndFrame();



		// Draw all ImGui windows, including the Viewport image
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and render additional platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			// for OpenGL: restore current GL context.
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		ImGuiIO& io = ImGui::GetIO();

		switch (event.GetEventType())
		{
			case EventType::KeyPressed:
			case EventType::KeyReleased:
			{
				if (io.WantCaptureKeyboard)
				{
					// imgui wants to capture this event
					// mark it as handled so it doesn't propogate down to other app layers.
					event.Handled = true;
				}
				break;
			}
			case EventType::MouseButtonPressed:
			case EventType::MouseButtonReleased:
			case EventType::MouseMoved:
			case EventType::MouseScrolled:
			{
				if (io.WantCaptureMouse)
				{
					event.Handled = true;
				}
				break;
			}
			case EventType::WindowResize:
			{
				WindowResizeEvent& e = (WindowResizeEvent&)event;

				// tell imgui the new app window size
				io.DisplaySize = ImVec2((float)e.GetWidth(), e.GetHeight());
				break;
			}
			default:
				break;
		}
	}

	// -------------------------IMGUI MODULE DEFINITIONS-------------------------

	// MODULE STATE BOOLS to keep track of shown/unshown modules
	static bool showInspectorModule = true;
	static bool showViewportModule = true;
	static bool showHierarchyModule = true;
	static bool showFileDirectoryModule = true;
	static bool showConsoleModule = true;
	static bool showControlsModule = true;

#define CHECKED_MENU_ITEM(menuItemName, checkedState) if (ImGui::MenuItem(menuItemName, NULL, checkedState)) { checkedState = !checkedState; }

	void ImGuiLayer::ShowMainMenuBar()
	{
		if (ImGui::BeginMainMenuBar())
		{
			// file menu bar option
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save", "CTRL + S")) {}
				if (ImGui::MenuItem("Save as")) {}
				if (ImGui::MenuItem("Import")) {}
				ImGui::EndMenu();
			}
			// settings menu bar option
			if (ImGui::BeginMenu("Settings"))
			{
				ImGui::EndMenu();
			}
			// view module menu bar option
			if (ImGui::BeginMenu("View Module"))
			{
				// show all module options to close/open specific modules
				CHECKED_MENU_ITEM("Inspector", showInspectorModule);
				CHECKED_MENU_ITEM("Viewport", showViewportModule);
				CHECKED_MENU_ITEM("Hierarchy", showHierarchyModule);
				CHECKED_MENU_ITEM("File Directory", showFileDirectoryModule);
				CHECKED_MENU_ITEM("Console", showConsoleModule);
				CHECKED_MENU_ITEM("Controls", showControlsModule);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}

	static float pos[3];
	static float rot[3];
	static float scale[3];
	static bool scaleUniform;

	void ImGuiLayer::ShowTransformComponent()
	{
		//TODO: transform component
		if (ImGui::CollapsingHeader("Transform"))
		{
			float oldScaleValues[3] = { scale[0], scale[1], scale[2] };

			// transform sliders
			ImGui::DragFloat3("Position", pos, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
			ImGui::DragFloat3("Rotation", rot, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");

			if (ImGui::DragFloat3("Scale", scale, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f"))
			{
				float deltaX = scale[0] - oldScaleValues[0];
				float deltaY = scale[1] - oldScaleValues[1];
				float deltaZ = scale[2] - oldScaleValues[2];

				// check if scale uniform is on
				if (scaleUniform)
				{
					// change all sliders the same amount as changed slider
					if (deltaX != 0)
					{
						scale[1] += deltaX;
						scale[2] += deltaX;
					}
					else if (deltaY != 0)
					{
						scale[0] += deltaY;
						scale[2] += deltaY;
					}
					else if (deltaZ != 0)
					{
						scale[0] += deltaZ;
						scale[1] += deltaZ;
					}
				}
			}
			ImGui::Checkbox("Scale Uniform", &scaleUniform);

		}
	}

	static bool isTrigger;

	void ImGuiLayer::ShowMeshColliderComponent()
	{
		if (ImGui::CollapsingHeader("Mesh Collider"))
		{
			ImGui::Checkbox("Is Trigger", &isTrigger);
		}
	}

	void ImGuiLayer::ShowInspectorModule()
	{

		if (showInspectorModule)
		{
			if (ImGui::Begin("Inspector", &showInspectorModule))
			{
				ShowTransformComponent();
				ShowMeshColliderComponent();
			}
			ImGui::End();
		}
	}

	GLuint viewportTexture;
	int viewportWidth = 800;
	int viewportHeight = 600;

	void ImGuiLayer::ShowViewportModule()
	{


		if (showViewportModule)
		{
			//if (ImGui::Begin("Viewport", &showViewportModule))
			//{
			//	ImVec2 size = ImGui::GetContentRegionAvail();

			//	ImGui::Image(
			//		(ImTextureID)(intptr_t)viewportTexture,
			//		size,
			//		ImVec2(0, 1),
			//		ImVec2(1, 0)
			//	);
			//}
			//ImGui::End();

			// Viewport window
			static ImVec2 viewportSize = ImVec2(0.0f, 0.0f);
			bool viewportHovered = false;
			bool viewportFocused = false;

			ImGui::Begin("Viewport");

			viewportHovered = ImGui::IsWindowHovered();
			viewportFocused = ImGui::IsWindowFocused();

			// Get the size available inside this window for content
			viewportSize = ImGui::GetContentRegionAvail();

			// Prevent weird zero-cases when minimized/collapsed
			if (viewportSize.x < 1.0f) viewportSize.x = 1.0f;
			if (viewportSize.y < 1.0f) viewportSize.y = 1.0f;

			// Resize the framebuffer if panel size changed
			// TODO: make viewportframebuffer static variable that is accessed through renderer facade
			Renderer::GetViewportFramebuffer()->Resize(
				static_cast<unsigned int>(viewportSize.x),
				static_cast<unsigned int>(viewportSize.y)
			);

			

			// Show the framebuffer's color texture inside ImGui
			// ImGui uses ImTextureID, and for OpenGL that is just the texture handle cast.
			// UVs are flipped vertically because OpenGL texture origin is bottom-left,
			// while ImGui expects top-left style display.
			ImGui::Image(
				(ImTextureID)(intptr_t)Renderer::GetViewportFramebuffer()->GetColorAttachment(),
				ImVec2(viewportSize.x, viewportSize.y),
				ImVec2(0, 1),   // UV top-left
				ImVec2(1, 0)    // UV bottom-right
			);

			// IMPORTANT for picking
			// Top-left of where the image will be drawn in screen coordinates
			m_ViewportImageMin = ImGui::GetCursorScreenPos();
			m_ViewportImageMax = ImVec2(
				m_ViewportImageMin.x + viewportSize.x,
				m_ViewportImageMin.y + viewportSize.y
			);

			ImGui::End();
		}
	}


	//testing 
	std::vector<HierarchyNode> hierarchy =
	{
		{"Scene",
			{
				{"Camera"},
				{"Player",
					{
						{"Weapon"},
						{"Mesh"}
					}
				},
				{"Light"}
			}
		}
	};

	void ImGuiLayer::ShowHierarchyModule()
	{
		if (showHierarchyModule)
		{
			if (ImGui::Begin("Hierarchy", &showHierarchyModule))
			{
				//TODO: myca
				if (ImGui::BeginTable("HierarchyTable", 1))
				{
					for (auto& node : hierarchy)
					{
						DrawHierarchyNode(node);
					}

					ImGui::EndTable();
				}
			}
			ImGui::End();
		}
	}

	void ImGuiLayer::DrawHierarchyNode(HierarchyNode& node)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		bool open = ImGui::TreeNode(node.name.c_str());

		if (open)
		{
			for (auto& child : node.children)
			{
				DrawHierarchyNode(child);
			}

			ImGui::TreePop();
		}

		if (ImGui::Button("Add primitive to scene")) {
			// temporary
			AddPrimitive();
		}
	}

	void ImGuiLayer::AddPrimitive() {

	}

	////holds the files
	//namespace fs = std::filesystem;
	//void ImGuiLayer::DrawDirectoryTree(const fs::path& path)
	//{
	//	for (const auto& entry : fs::directory_iterator(path))
	//	{
	//		const auto& p = entry.path();
	//		std::string name = p.filename().string();
	//		if (entry.is_directory())
	//		{
	//			if (ImGui::TreeNode(name.c_str()))
	//			{
	//				DrawDirectoryTree(p);
	//				ImGui::TreePop();
	//			}
	//		}
	//		else
	//		{
	//			ImGui::BulletText("%s", name.c_str());
	//		}
	//	}
	//}

	////search bar for files
	//void ImGuiLayer::DrawDirectorySearch(const fs::path& path, ImGuiTextFilter& filter)
	//{
	//	for (const auto& entry : fs::recursive_directory_iterator(path))
	//	{
	//		std::string name = entry.path().filename().string();

	//		if (filter.PassFilter(name.c_str()))
	//		{
	//			if (ImGui::Selectable(name.c_str()))
	//			{

	//			}
	//		}
	//	}
	//}

	////puts them together
	//void ImGuiLayer::DrawDirectory(const fs::path& path)
	//{
	//	static ImGuiTextFilter filter;
	//	filter.Draw("Search");
	//	bool searching = strlen(filter.InputBuf) > 0;
	//	if (searching)
	//		DrawDirectorySearch(path, filter);
	//	else
	//		DrawDirectoryTree(path);
	//}

	//void ImGuiLayer::ShowFileDirectoryModule()
	//{
	//	if (showFileDirectoryModule)
	//	{
	//		if (ImGui::Begin("File Directory", &showFileDirectoryModule))
	//		{
	//			//used the helper cause i thought it would be helpful lol
	//			// HelpMarker("This showes the files in the project, you can use the search bar to find a specific files");

	//			DrawDirectory(".");

	//		}
	//		ImGui::End();
	//	}
	//}

	void ImGuiLayer::ShowConsoleTraceOutput(const char* source, const char* message)
	{
		ImU32 consoleTraceGray = ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 0.65f));
		ImGui::Text("[%s] : %s", source, message);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, consoleTraceGray);
	}

	void ImGuiLayer::ShowConsoleWarningOutput(const char* source, const char* message)
	{
		ImU32 consoleWarningYellow = ImGui::GetColorU32(ImVec4(0.6f, 0.4f, 0.04f, 0.85f));
		ImGui::Text("[%s] : %s", source, message);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, consoleWarningYellow);
	}

	void ImGuiLayer::ShowConsoleErrorOutput(const char* source, const char* message)
	{
		ImU32 consoleErrorRed = ImGui::GetColorU32(ImVec4(0.8f, 0.1f, 0.04f, 0.65f));
		ImGui::Text("[%s] : %s", source, message);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, consoleErrorRed);
	}

	void ImGuiLayer::ShowConsoleModule()
	{
		if (showConsoleModule)
		{
			if (ImGui::Begin("Console", &showConsoleModule))
			{
				static ImGuiTextFilter filter;
				filter.Draw("Search");
				//const char* lines[] = { "aaa1.c", "bbb1.c", "ccc1.c", "aaa2.cpp", "bbb2.cpp", "ccc2.cpp", "abc.h", "hello, world" };
				//for (int i = 0; i < IM_COUNTOF(lines); i++)
				//	if (filter.PassFilter(lines[i]))
				//		ImGui::BulletText("%s", lines[i]);
				//ImGui::TreePop();

				if (ImGui::BeginTable("ConsoleOutputTable", 1))
				{
					ImGui::TableSetupColumn("Output", ImGuiTableColumnFlags_WidthFixed);

					ImGui::TableNextRow();
					if (ImGui::TableSetColumnIndex(0))
					{
						ShowConsoleTraceOutput("TEST", "This is an example trace output.");
					}

					ImGui::TableNextRow();
					if (ImGui::TableNextColumn())
					{
						ShowConsoleWarningOutput("TEST", "This is an example warning output.");
					}

					ImGui::TableNextRow();
					if (ImGui::TableNextColumn())
					{
						ShowConsoleErrorOutput("TEST", "This is an example error output!");
					}

					for (int i = 0; i < 50; i++) {
						ImGui::TableNextRow();
						if (ImGui::TableNextColumn())
						{
							ShowConsoleTraceOutput("TEST", std::format("Output {}", i).c_str());
						}
					}

				}
				ImGui::EndTable();
			}
			ImGui::End();
		}
	}

	void ImGuiLayer::ShowControlsModule()
	{
		// control keybinds - hardcoded because not planning on being customizable
		std::vector<std::pair<const char*, const char*>> ControlTextMap =
		{
			{"Undo", "CTRL + Z"},
			{"Redo", "CTRL + Y"},
			{"Rotate Viewport Angle", "ALT + MMB"},
			{"Zoom In","Mouse Scroll Down"},
			{"Zoom Out","Mouse Scroll Up"},
			{"Create Camera From View","CTRL + SHIFT + C"},
			{"Move","W"},
			{"Rotate","E"},
			{"Scale","R"},
			{"Duplicate","CTRL + D"},
			{"Delete","DELETE"}
		};

		if (showControlsModule)
		{
			if (ImGui::Begin("Controls", &showControlsModule))
			{
				// HelpMarker("Control keybinds in Orion are not currently editable!");
				static ImGuiTableFlags flags =
					ImGuiTableFlags_SizingFixedFit |
					ImGuiTableFlags_Hideable;

				if (ImGui::BeginTable("ControlsTable", 3, flags))
				{
					// make control name column fixed width, keybind column stretch
					ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Keybind", ImGuiTableColumnFlags_WidthStretch);
					auto it = ControlTextMap.begin();

					// set up all row entries
					for (int row = 0; row < ControlTextMap.size(); row++)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text(it->first);
						ImGui::TableSetColumnIndex(1);
						ImGui::Text(it->second);

						// iterate to next control keybind to print
						it++;
					}
					ImGui::EndTable();
				}
				ImGui::End();
			}
		}
	}

}
