#define NOMINMAX	// resolves std::max error
#if defined(_WIN32) || defined(ORN_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN   // exclude rarely-used Windows headers (avoids prsht.h etc.)
#include <windows.h>          // must come before commdlg.h
#include <commdlg.h>          // GetSaveFileName / GetOpenFileName
#endif
#include "EngineCore.h"
#include "Layers/ImGuiLayer.h"
#include "Layers/EditorLayer.h"
#include "Application.h"
#include "Renderer/Renderer.h"
#include "ECS/SceneManager.h"
#include "Assets/AssetManager.h"
#include "Core/ProjectSettings.h"

#include "imgui.h"
#include "imgui_internal.h"   // ClearActiveID
#include "libintl.h"

// #include "Platform/OpenGL/ImGuiOpenGLRenderer.h"
#include "../external/ImGUI/backends/imgui_impl_glfw.h"
#include "../external/ImGUI/backends/imgui_impl_opengl3.h"

// TODO: remove glfw include here
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <nlohmann/json.hpp>

#include <format>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>   // ShellExecuteA
#endif

#define IMGUI_ELEMENT_TITLE(displayName, id) std::string(_(displayName) + std::format("###{}", id)).c_str()

namespace Orion {

	// Initialize static members
	EntityID ImGuiLayer::s_HoveredEntity = INVALID_ENTITY;

	bool ImGuiLayer::s_ViewportHovered = false;
	bool ImGuiLayer::s_ViewportFocused = false;
	bool ImGuiLayer::s_ViewportDragging = false;
	ImVec2 ImGuiLayer::s_ViewportImageMin = { 0, 0 };
	ImVec2 ImGuiLayer::s_ViewportImageMax = { 0, 0 };

	std::string ImGuiLayer::s_NotificationMessage;
	float       ImGuiLayer::s_NotificationTimer = 0.0f;

	std::vector<ImGuiLayer::ConsoleEntry> ImGuiLayer::s_ConsoleEntries;

	void ImGuiLayer::AddConsoleMessage(ConsoleEntry::Level level, const char* source, const char* message)
	{
		s_ConsoleEntries.push_back({ level, source ? source : "", message ? message : "" });
	}

	void ImGuiLayer::ClearConsole()
	{
		s_ConsoleEntries.clear();
	}

	void ImGuiLayer::ShowNotification(const std::string& message, float duration)
	{
		s_NotificationMessage = message;
		s_NotificationTimer   = duration;
	}

	std::string ImGuiLayer::ShowSaveFileDialog(const std::string& defaultDir,
	                                            const std::string& defaultName)
	{
#if defined(_WIN32)
		char fileName[MAX_PATH] = {};
		// Pre-fill the filename box with the suggested name (lpstrDefExt appends .scene if omitted)
		strncpy_s(fileName, (defaultName + ".scene").c_str(), MAX_PATH - 1);

		// Resolve the initial directory: prefer the supplied defaultDir, then the
		// assets folder, then fall back to %USERPROFILE%\Documents.
		std::string initDir = defaultDir;
		if (initDir.empty())
			initDir = AssetManager::GetAssetsFolderPath();
		if (initDir.empty()) {
			char userProfile[MAX_PATH] = {};
			if (GetEnvironmentVariableA("USERPROFILE", userProfile, MAX_PATH) > 0)
				initDir = std::string(userProfile) + "\\Documents";
		}

		OPENFILENAMEA ofn   = {};
		ofn.lStructSize     = sizeof(ofn);
		ofn.hwndOwner       = nullptr;   // no explicit owner — dialog is still modal to the process
		ofn.lpstrFilter     = "Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0";
		ofn.lpstrFile       = fileName;
		ofn.nMaxFile        = MAX_PATH;
		ofn.lpstrDefExt     = "scene";
		ofn.lpstrInitialDir = initDir.empty() ? nullptr : initDir.c_str();
		ofn.lpstrTitle      = "Save Scene";
		ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetSaveFileNameA(&ofn))
			return std::string(fileName);
#endif
		return "";
	}


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

		// Defer ini path setup — assets folder isn't configured until Application::Run().
		// We set IniFilename to null here so ImGui doesn't try to load a stale path,
		// then set the real path on the first frame in OnUpdate().
		io.IniFilename = nullptr;

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

		// -------------------------------- ImGui ------------------------------------

		// Deferred UI layout setup: on the first frame, AssetManager's path is ready,
		// so we can set the ini file path and load any saved layout.
		static bool iniLoaded = false;
		if (!iniLoaded) {
			std::string assetsPath = AssetManager::GetAssetsFolderPath();
			if (!assetsPath.empty()) {
				static std::string iniPath = assetsPath + "imgui_layout.ini";
				ImGui::GetIO().IniFilename = iniPath.c_str();
				if (std::filesystem::exists(iniPath))
					ImGui::LoadIniSettingsFromDisk(iniPath.c_str());
				iniLoaded = true;
			}
		}


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
		//ImGui::ShowDemoWindow(&show);
		ShowMainMenuBar();
		ShowInspectorModule();
		ShowViewportModule();
		ShowHierarchyModule();
		ShowAssetBrowser();
		// ShowConsoleModule();
		ShowConsoleModule();
		ShowControlsModule();
		ShowProjectSettingsWindow();

		// Tell ImGui to finalize all UI draw data
		ImGui::Render();

		// Object Picking:
		// TODO: mousePos is being overwritten so this bool will always be false. Get mouse input through event system
		ImVec2 mousePos = ImGui::GetMousePos();
		bool mouseInsideViewportImage =
			mousePos.x >= s_ViewportImageMin.x &&
			mousePos.x < s_ViewportImageMax.x &&
			mousePos.y >= s_ViewportImageMin.y &&
			mousePos.y < s_ViewportImageMax.y;
		if (mouseInsideViewportImage) {
			int localMouseX = static_cast<int>(mousePos.x - s_ViewportImageMin.x);
			int localMouseY = static_cast<int>(mousePos.y - s_ViewportImageMin.y);
			// std::cout << localMouseX << "," << localMouseY << std::endl;
			s_HoveredEntity = Renderer::PickEntity(localMouseX, localMouseY);
			// std::cout << "Hovered entity: " << hovered << " at (" << localMouseX << "," << localMouseY << ")\n";

		}
		else {
			// std::cout << "Hovered entity: 000\n";
			// std::cout << "Outside viewport" << std::endl;
		}




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
			// Block keyboard events only when ImGui truly needs them
			// (e.g. typing in an InputText) AND the user isn't actively
			// interacting with the viewport (camera fly uses WASD).
			// Never block Ctrl+key combos — they are global editor shortcuts
			// (Ctrl+S, Ctrl+Z, Ctrl+Y, etc.) that must reach EditorLayer regardless
			// of which panel has focus.
			if (io.WantCaptureKeyboard && !s_ViewportFocused && !s_ViewportDragging && !io.KeyCtrl)
			{
				event.Handled = true;
			}
			break;
		}
		case EventType::MouseButtonPressed:
		{
			// If a mouse press starts in the viewport, begin a viewport drag.
			// This keeps events flowing to EditorLayer even if the cursor
			// leaves the viewport mid-drag (e.g. camera fly with RMB).
			if (s_ViewportHovered)
				s_ViewportDragging = true;

			if (io.WantCaptureMouse && !s_ViewportHovered)
				event.Handled = true;
			break;
		}
		case EventType::MouseButtonReleased:
		{
			// Capture whether a viewport drag was active before clearing it.
			// If the drag started in the viewport, the release must reach EditorLayer
			// even if the cursor is now outside — otherwise camera/gizmo state stays stuck.
			bool wasDragging = s_ViewportDragging;
			s_ViewportDragging = false;

			if (io.WantCaptureMouse && !s_ViewportHovered && !wasDragging)
				event.Handled = true;
			break;
		}
		case EventType::MouseMoved:
		case EventType::MouseScrolled:
		{
			// Allow mouse events through if the cursor is over the viewport
			// OR if a drag started in the viewport (camera fly, gizmo drag, etc.).
			if (io.WantCaptureMouse && !s_ViewportHovered && !s_ViewportDragging)
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
	static bool showProjectSettings = false;
	static bool showBuildGamePopup = false;

#define CHECKED_MENU_ITEM(menuItemName, checkedState) if (ImGui::MenuItem(menuItemName, NULL, checkedState)) { checkedState = !checkedState; }

	void ImGuiLayer::ShowMainMenuBar()
	{
		if (ImGui::BeginMainMenuBar())
		{
			// file menu bar option
			if (ImGui::BeginMenu(IMGUI_ELEMENT_TITLE("File", "File")))
			{
				if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Save", "Save"), "CTRL + S")) {
					const std::string& path = SceneManager::GetActiveScenePath();
					if (!path.empty()) {
						if (SceneManager::SaveScene(path))
							ShowNotification("Saved: " + std::filesystem::path(path).filename().string());
						else
							ShowNotification("Save failed — check write permissions.", 5.0f);
					} else {
						// No path yet — fall through to Save As
						std::string newPath = ShowSaveFileDialog(AssetManager::GetAssetsFolderPath());
						if (!newPath.empty()) {
							if (SceneManager::SaveScene(newPath))
								ShowNotification("Saved: " + std::filesystem::path(newPath).filename().string());
							else
								ShowNotification("Save failed — check write permissions.", 5.0f);
						}
					}
				}
				if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Save as", "Save as"), "CTRL + SHIFT + S")) {
					const std::string& currentPath = SceneManager::GetActiveScenePath();
					std::string defaultDir  = currentPath.empty()
					    ? AssetManager::GetAssetsFolderPath()
					    : std::filesystem::path(currentPath).parent_path().string();
					std::string defaultName = currentPath.empty()
					    ? "untitled"
					    : std::filesystem::path(currentPath).stem().string();
					std::string newPath = ShowSaveFileDialog(defaultDir, defaultName);
					if (!newPath.empty()) {
						if (SceneManager::SaveScene(newPath))
							ShowNotification("Saved: " + std::filesystem::path(newPath).filename().string());
						else
							ShowNotification("Save failed — check write permissions.", 5.0f);
					}
				}
				ImGui::Separator();
				if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Build Game...", "BuildGame"))) {
					showBuildGamePopup = true;
				}
				ImGui::EndMenu();
			}
			// settings menu bar option
			if (ImGui::BeginMenu(IMGUI_ELEMENT_TITLE("Settings", "Settings")))
			{
				if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Project Settings", "Project Settings")))
					showProjectSettings = true;
				ImGui::EndMenu();
			}
			// view module menu bar option
			if (ImGui::BeginMenu(IMGUI_ELEMENT_TITLE("View Module", "ModuleMenu")))
			{
				// show all module options to close/open specific modules
				CHECKED_MENU_ITEM(IMGUI_ELEMENT_TITLE("Inspector", "InspectorCheck"), showInspectorModule);
				CHECKED_MENU_ITEM(IMGUI_ELEMENT_TITLE("Viewport", "ViewportCheck"), showViewportModule);
				CHECKED_MENU_ITEM(IMGUI_ELEMENT_TITLE("Hierarchy", "HierarchyCheck"), showHierarchyModule);
				CHECKED_MENU_ITEM(IMGUI_ELEMENT_TITLE("File Directory", "FileDirectoryCheck"), showFileDirectoryModule);
				CHECKED_MENU_ITEM(IMGUI_ELEMENT_TITLE("Console", "ConsoleCheck"), showConsoleModule);
				CHECKED_MENU_ITEM(IMGUI_ELEMENT_TITLE("Controls", "ControlsCheck"), showControlsModule);
				ImGui::Separator();
				if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Reset Layout", "ResetLayout"))) {
					ImGuiIO& menuIO = ImGui::GetIO();
					if (menuIO.IniFilename)
						std::filesystem::remove(menuIO.IniFilename);
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		// ========================== BUILD GAME POPUP ==========================
		if (showBuildGamePopup) {
			ImGui::OpenPopup(IMGUI_ELEMENT_TITLE("Build Game", "BuildGamePopup"));
			showBuildGamePopup = false;
		}

		if (ImGui::BeginPopupModal(IMGUI_ELEMENT_TITLE("Build Game", "BuildGamePopup"), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static char gameName[128] = "My Game";
			static char outputDir[512] = "";
			static int resolutionIdx = 0;
			static bool fullscreen = false;
			static int sceneIdx = 0;
			static std::string buildStatus;

			// Collect .scene files from assets
			static std::vector<std::string> sceneFiles;
			static bool scenesScanned = false;
			if (!scenesScanned) {
				sceneFiles.clear();
				std::string assetsPath = AssetManager::GetAssetsFolderPath();
				if (!assetsPath.empty()) {
					for (auto& entry : std::filesystem::recursive_directory_iterator(assetsPath)) {
						if (entry.is_regular_file() && entry.path().extension() == ".scene") {
							// Store relative to assets folder
							std::string rel = entry.path().string().substr(assetsPath.size());
							std::replace(rel.begin(), rel.end(), '\\', '/');
							sceneFiles.push_back(rel);
						}
					}
				}
				scenesScanned = true;
			}

			ImGui::Text(_("Build Game"));
			ImGui::Separator();

			ImGui::InputText(IMGUI_ELEMENT_TITLE("Game Name", "Game Name"), gameName, sizeof(gameName));

			// Start Scene dropdown
			if (ImGui::BeginCombo(IMGUI_ELEMENT_TITLE("Start Scene", "Start Scene"), sceneIdx < (int)sceneFiles.size() ? sceneFiles[sceneIdx].c_str() : "(none)")) {
				for (int i = 0; i < (int)sceneFiles.size(); i++) {
					if (ImGui::Selectable(sceneFiles[i].c_str(), i == sceneIdx))
						sceneIdx = i;
				}
				ImGui::EndCombo();
			}

			// Resolution presets
			const char* resolutions[] = { "1920 x 1080", "1280 x 720", "2560 x 1440", "800 x 600" };
			const int resW[] = { 1920, 1280, 2560, 800 };
			const int resH[] = { 1080, 720, 1440, 600 };
			ImGui::Combo(IMGUI_ELEMENT_TITLE("Resolution", "GameResolution"), &resolutionIdx, resolutions, 4);

			ImGui::Checkbox(IMGUI_ELEMENT_TITLE("Fullscreen", "FullscreenCheck"), &fullscreen);

			ImGui::InputText(IMGUI_ELEMENT_TITLE("Output Folder", "Output Folder"), outputDir, sizeof(outputDir));
			ImGui::SameLine();
			if (ImGui::Button("...")) {
				// Default to a reasonable location
#ifdef _WIN32
				std::string defaultPath = std::string(getenv("USERPROFILE") ? getenv("USERPROFILE") : "C:\\") + "\\Desktop\\" + gameName;
				strncpy_s(outputDir, defaultPath.c_str(), sizeof(outputDir) - 1);
#endif
			}

			ImGui::Separator();

			if (!buildStatus.empty()) {
				ImGui::TextWrapped("%s", buildStatus.c_str());
				ImGui::Separator();
			}

			float buttonWidth = 120.0f;
			if (ImGui::Button(IMGUI_ELEMENT_TITLE("Build", "BuildGame"), ImVec2(buttonWidth, 0))) {
				buildStatus.clear();

				if (strlen(outputDir) == 0) {
					buildStatus = _("Error: Please set an output folder.");
				}
				else if (sceneFiles.empty()) {
					buildStatus = _("Error: No .scene files found in assets.");
				}
				else {
					namespace fs = std::filesystem;
					try {
						fs::path out(outputDir);
						fs::create_directories(out);

						// 1. Find Runtime.exe next to Editor.exe
						fs::path exeDir = fs::path(fs::current_path()); // working dir is bin/
						// Try common locations for Runtime.exe
						fs::path runtimeExe;
						if (fs::exists(exeDir / "Runtime.exe"))
							runtimeExe = exeDir / "Runtime.exe";
						else if (fs::exists(exeDir / ".." / "Runtime.exe"))
							runtimeExe = exeDir / ".." / "Runtime.exe";
						else {
							// Search the build output directory
							for (auto& e : fs::recursive_directory_iterator(exeDir / "..")) {
								if (e.path().filename() == "Runtime.exe") {
									runtimeExe = e.path();
									break;
								}
							}
						}

						if (runtimeExe.empty() || !fs::exists(runtimeExe)) {
							buildStatus = _("Error: Runtime.exe not found. Build the Runtime target first.");
						}
						else {
							// Layout mirrors the editor structure:
							//   out/bin/GameName.exe   (working dir)
							//   out/engine/shaders/    (accessed as ../engine/shaders/)
							//   out/editor/assets/     (accessed as ../editor/assets/)

							// 2. Copy Runtime.exe as GameName.exe into bin/
							fs::path binDir = out / "bin";
							fs::create_directories(binDir);
							fs::copy_file(runtimeExe, binDir / (std::string(gameName) + ".exe"), fs::copy_options::overwrite_existing);

							// 3. Copy Engine.dll into bin/
							fs::path engineDll = runtimeExe.parent_path() / "Engine.dll";
							if (fs::exists(engineDll))
								fs::copy_file(engineDll, binDir / "Engine.dll", fs::copy_options::overwrite_existing);

							// 4. Copy GNU.Gettext.dll into bin/
							fs::path gettextDll = runtimeExe.parent_path() / "GNU.Gettext.dll";
							if (!fs::exists(gettextDll))
								gettextDll = exeDir / "GNU.Gettext.dll";
							if (fs::exists(gettextDll))
								fs::copy_file(gettextDll, binDir / "GNU.Gettext.dll", fs::copy_options::overwrite_existing);

							// 5. Write game.settings into bin/ (next to the exe)
							std::string selectedScene = sceneIdx < (int)sceneFiles.size() ? sceneFiles[sceneIdx] : "default.scene";
							std::replace(selectedScene.begin(), selectedScene.end(), '/', '\\');

							nlohmann::json j;
							j["title"] = gameName;
							j["width"] = resW[resolutionIdx];
							j["height"] = resH[resolutionIdx];
							j["fullscreen"] = fullscreen;
							j["startScene"] = selectedScene;

							std::ofstream settingsFile((binDir / "game.settings").string());
							if (settingsFile.is_open()) {
								settingsFile << j.dump(4);
								settingsFile.close();
							}

							// 6. Copy shaders into engine/shaders/ (accessed as ../engine/shaders/)
							fs::path shadersDir = exeDir / ".." / "engine" / "shaders";
							if (fs::exists(shadersDir)) {
								fs::path destShaders = out / "engine" / "shaders";
								fs::create_directories(destShaders);
								for (auto& entry : fs::directory_iterator(shadersDir)) {
									if (entry.is_regular_file())
										fs::copy_file(entry.path(), destShaders / entry.path().filename(), fs::copy_options::overwrite_existing);
								}
							}

							// 7. Copy engine built-in assets
							fs::path engineAssetsDir = exeDir / ".." / "engine" / "engineAssets";
							if (fs::exists(engineAssetsDir)) {
								fs::path destEA = out / "engine" / "engineAssets";
								fs::create_directories(destEA);
								fs::copy(engineAssetsDir, destEA, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
							}

							// 8. Copy locales
							fs::path localesDir = exeDir / ".." / "engine" / "locales";
							if (fs::exists(localesDir)) {
								fs::path destLoc = out / "engine" / "locales";
								fs::create_directories(destLoc);
								fs::copy(localesDir, destLoc, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
							}

							// 9. Copy all project assets into editor/assets/
							// (Runtime falls back to ../editor/assets/ which matches this layout)
							std::string assetsPath = AssetManager::GetAssetsFolderPath();
							fs::path destAssets = out / "editor" / "assets";
							fs::create_directories(destAssets);
							fs::copy(fs::path(assetsPath), destAssets, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

							buildStatus = "Build complete! Output: " + out.string();

#ifdef _WIN32
							ShellExecuteA(NULL, "explore", out.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
						}
					}
					catch (const std::exception& e) {
						buildStatus = std::string("Error: ") + e.what();
					}
				}
			}

			ImGui::SameLine();
			if (ImGui::Button(IMGUI_ELEMENT_TITLE("Close", "CloseBuildGamePopup"), ImVec2(buttonWidth, 0))) {
				buildStatus.clear();
				scenesScanned = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	// ========================== INSPECTOR ==========================

	// Helper: human-readable name for each component type
	static const char* ComponentTypeName(ComponentType type)
	{
		switch (type) {
		case ComponentType::EntityData: return "Entity Data";
		case ComponentType::Transform:  return "Transform";
		case ComponentType::Mesh:       return "Mesh";
		case ComponentType::Material:   return "Material";
		case ComponentType::Camera:     return "Camera";
		case ComponentType::Script:     return "Script";
		case ComponentType::Rigidbody:  return "Rigidbody";
		case ComponentType::Collider:   return "Collider";
		case ComponentType::PointLight:    return "Point Light";
		case ComponentType::AudioSource:   return "Audio Source";
		case ComponentType::AudioListener: return "Audio Listener";
		default:                           return "Unknown";
		}
	}

	bool ImGuiLayer::IsRequiredComponent(ComponentType type)
	{
		// EntityData and Transform are mandatory — every entity must have them.
		return type == ComponentType::EntityData || type == ComponentType::Transform;
	}

	void ImGuiLayer::RebuildComponentOrder(EntityID entity, Scene& scene)
	{
		// List every component the entity actually owns, in default order.
		std::vector<ComponentType> order;

		if (scene.HasEntityDataComponent(entity))
			order.push_back(ComponentType::EntityData);
		if (scene.HasTransformComponent(entity))
			order.push_back(ComponentType::Transform);
		if (scene.HasMeshComponent(entity))
			order.push_back(ComponentType::Mesh);
		if (scene.HasMaterialComponent(entity))
			order.push_back(ComponentType::Material);
		if (scene.HasCameraComponent(entity))
			order.push_back(ComponentType::Camera);
		if (scene.HasScriptComponent(entity))
			order.push_back(ComponentType::Script);
		if (scene.HasRigidbodyComponent(entity))
			order.push_back(ComponentType::Rigidbody);
		if (scene.HasColliderComponent(entity))
			order.push_back(ComponentType::Collider);
		if (scene.HasPointLightComponent(entity))
			order.push_back(ComponentType::PointLight);
		if (scene.HasAudioSourceComponent(entity))
			order.push_back(ComponentType::AudioSource);
		if (scene.HasAudioListenerComponent(entity))
			order.push_back(ComponentType::AudioListener);

		m_ComponentOrder[entity] = order;
	}

	// ---------- Individual component field drawers ----------

	void ImGuiLayer::DrawEntityDataFields(EntityID entity, Scene& scene)
	{
		EntityDataComponent* data = scene.GetEntityDataComponent(entity);
		if (!data) return;

		// Editable entity name
		char nameBuf[256];
		strncpy_s(nameBuf, data->name.c_str(), sizeof(nameBuf) - 1);
		nameBuf[sizeof(nameBuf) - 1] = '\0';
		if (ImGui::InputText(IMGUI_ELEMENT_TITLE("Name", "EntityNameField"), nameBuf, sizeof(nameBuf))) {
			// Don't allow an empty name — it breaks the hierarchy display
			if (nameBuf[0] != '\0')
				data->name = nameBuf;
		}

		ImGui::Checkbox(IMGUI_ELEMENT_TITLE("Enabled", "EnabledEntity"), &data->enabled);
	}

	void ImGuiLayer::DrawTransformFields(EntityID entity, Scene& scene)
	{
		TransformComponent* tc = scene.GetTransformComponent(entity);
		if (!tc) return;

		ImGui::DragFloat3(IMGUI_ELEMENT_TITLE("Position", "Position"), &tc->position.x, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");

		// Display rotation in degrees, store in radians
		glm::vec3 rotDeg = glm::degrees(tc->rotation);
		if (ImGui::DragFloat3(IMGUI_ELEMENT_TITLE("Rotation", "Rotation"), &rotDeg.x, 0.1f, -FLT_MAX, +FLT_MAX, "%.1f"))
			tc->rotation = glm::radians(rotDeg);

		// Uniform-scale toggle — state stored per-inspector-session, reset on entity change
		glm::vec3 oldScale = tc->scale;

		if (ImGui::DragFloat3(IMGUI_ELEMENT_TITLE("Scale", "Scale"), &tc->scale.x, 0.005f, 0.001f, +FLT_MAX, "%.3f"))
		{
			// Clamp each axis away from zero so physics/renderer never receive a degenerate scale
			tc->scale.x = std::max(tc->scale.x, 0.001f);
			tc->scale.y = std::max(tc->scale.y, 0.001f);
			tc->scale.z = std::max(tc->scale.z, 0.001f);

			if (m_ScaleUniform) {
				glm::vec3 delta = tc->scale - oldScale;
				if (delta.x != 0.0f) {
					tc->scale.y = std::max(oldScale.y + delta.x, 0.001f);
					tc->scale.z = std::max(oldScale.z + delta.x, 0.001f);
				}
				else if (delta.y != 0.0f) {
					tc->scale.x = std::max(oldScale.x + delta.y, 0.001f);
					tc->scale.z = std::max(oldScale.z + delta.y, 0.001f);
				}
				else if (delta.z != 0.0f) {
					tc->scale.x = std::max(oldScale.x + delta.z, 0.001f);
					tc->scale.y = std::max(oldScale.y + delta.z, 0.001f);
				}
			}
		}
		ImGui::Checkbox(IMGUI_ELEMENT_TITLE("Scale Uniform", "Scale Uniform"), &m_ScaleUniform);
	}

	void ImGuiLayer::DrawMeshFields(EntityID entity, Scene& scene)
	{
		MeshComponent* mc = scene.GetMeshComponent(entity);
		if (!mc) return;

		// Build the preview label for the dropdown (current selection)
		std::string previewLabel = _("(none)");
		if (mc->mesh != INVALID_ASSET_ID) {
			MeshAsset* current = AssetManager::GetMeshAsset(mc->mesh);
			if (current && !current->name.empty())
				previewLabel = current->name;
			else
				previewLabel = "ID: " + std::to_string(mc->mesh);
		}

		// Dropdown to pick a mesh asset
		if (ImGui::BeginCombo(IMGUI_ELEMENT_TITLE("Mesh", "Mesh"), previewLabel.c_str()))
		{
			// Option to clear the mesh
			if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("(none)", "MeshNone"), mc->mesh == INVALID_ASSET_ID))
				mc->mesh = INVALID_ASSET_ID;

			// List every loaded mesh asset
			for (auto& [id, asset] : AssetManager::GetAllMeshAssets())
			{
				bool isSelected = (mc->mesh == id);
				std::string label = asset.name.empty()
					? ("ID: " + std::to_string(id))
					: asset.name;

				if (ImGui::Selectable(label.c_str(), isSelected))
					mc->mesh = id;

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// Show path as read-only info below the dropdown
		if (mc->mesh != INVALID_ASSET_ID) {
			MeshAsset* asset = AssetManager::GetMeshAsset(mc->mesh);
			if (asset)
			{
				std::string assetPath = _("Path") + std::format(": {}", asset->filePath);
				ImGui::TextDisabled(assetPath.c_str());
			}
		}
	}

	void ImGuiLayer::DrawMaterialFields(EntityID entity, Scene& scene)
	{
		MaterialComponent* matComp = scene.GetMaterialComponent(entity);
		if (!matComp) return;

		// Build the preview label for the dropdown (current selection)
		std::string previewLabel = _("(none)");
		if (matComp->material != INVALID_ASSET_ID) {
			MaterialAsset* current = AssetManager::GetMaterialAsset(matComp->material);
			if (current && !current->name.empty())
				previewLabel = current->name;
			else
				previewLabel = "ID: " + std::to_string(matComp->material);
		}

		// Dropdown to pick a material asset
		if (ImGui::BeginCombo(IMGUI_ELEMENT_TITLE("Material", "MaterialCombo"), previewLabel.c_str()))
		{
			// Option to clear the material
			if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("(none)", "MaterialNone"), matComp->material == INVALID_ASSET_ID))
				matComp->material = INVALID_ASSET_ID;

			// List every loaded material asset.
			// Label = "name (file.mtl)" so materials from different files
			// with the same newmtl name are visually distinguishable.
			for (auto& [id, asset] : AssetManager::GetAllMaterialAssets())
			{
				bool isSelected = (matComp->material == id);

				std::string label;
				if (asset.name.empty()) {
					label = "ID: " + std::to_string(id);
				} else {
					// Extract just the filename from the full key path
					std::string keyPath = AssetManager::GetMaterialPath(id);
					size_t sep = keyPath.find("::");
					std::string filename = (sep != std::string::npos)
						? std::filesystem::path(keyPath.substr(0, sep)).filename().string()
						: "";
					label = filename.empty()
						? asset.name
						: asset.name + " (" + filename + ")";
				}

				// Use id-suffixed ImGui ID so entries with identical labels don't collide
				std::string imguiLabel = label + "###MatSel" + std::to_string(id);
				if (ImGui::Selectable(imguiLabel.c_str(), isSelected))
					matComp->material = id;

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// Show material name hint below the dropdown
		if (matComp->material != INVALID_ASSET_ID) {
			MaterialAsset* asset = AssetManager::GetMaterialAsset(matComp->material);
			if (asset)
				ImGui::TextDisabled("Edit in Assets browser");
		}
	}

	void ImGuiLayer::DrawCameraFields(EntityID entity, Scene& scene)
	{
		CameraComponent* cam = scene.GetCameraComponent(entity);
		if (!cam) return;

		ImGui::DragFloat(IMGUI_ELEMENT_TITLE("FOV (degrees)", "FOV"), &cam->fovDegrees, 0.5f, 1.0f, 179.0f);
		ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Near Plane", "Near Plane"), &cam->nearPlane, 0.01f, 0.001f, 100.0f, "%.3f");
		ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Far Plane", "Far Plane"), &cam->farPlane, 1.0f, 1.0f, 10000.0f);
		ImGui::Checkbox(IMGUI_ELEMENT_TITLE("Active", "CamActive"), &cam->isActive);
	}

	void ImGuiLayer::DrawScriptFields(EntityID entity, Scene& scene)
	{
		ScriptComponent* sc = scene.GetScriptComponent(entity);
		if (!sc) return;

		// Editable script path
		char pathBuf[512];
		strncpy_s(pathBuf, sc->scriptPath.c_str(), sizeof(pathBuf) - 1);
		pathBuf[sizeof(pathBuf) - 1] = '\0';
		if (ImGui::InputText(IMGUI_ELEMENT_TITLE("Script Path", "Script Path"), pathBuf, sizeof(pathBuf))) {
			sc->scriptPath = pathBuf;
		}

		if (sc->scriptPath.empty()) {
			ImGui::TextDisabled(IMGUI_ELEMENT_TITLE("(no script assigned)", "ScriptPathNone"));
		}
	}

	void ImGuiLayer::DrawRigidbodyFields(EntityID entity, Scene& scene)
	{
		RigidbodyComponent* rb = scene.GetRigidbodyComponent(entity);
		if (!rb) return;

		// Body type dropdown
		const char* bodyTypes[] = { _("Static"), _("Kinematic"), _("Dynamic") };
		int currentType = static_cast<int>(rb->bodyType);
		if (ImGui::Combo(IMGUI_ELEMENT_TITLE("Body Type", "Body Type"), &currentType, bodyTypes, IM_ARRAYSIZE(bodyTypes))) {
			rb->bodyType = static_cast<BodyType>(currentType);
		}

		// Only show mass for dynamic bodies
		if (rb->bodyType == BodyType::Dynamic) {
			ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Mass", "Mass"), &rb->mass, 0.1f, 0.01f, 10000.0f, "%.2f");
		}

		ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Linear Damping", "Linear Damping"), &rb->linearDamping, 0.01f, 0.0f, 100.0f, "%.3f");
		ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Angular Damping", "Angular Damping"), &rb->angularDamping, 0.01f, 0.0f, 100.0f, "%.3f");
		ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Gravity Scale", "Gravity Scale"), &rb->gravityScale, 0.05f, 0.0f, 10.0f, "%.2f");

		ImGui::Separator();
		ImGui::Text(_("Freeze Rotation"));
		ImGui::Checkbox("X##FreezeRotX", &rb->freezeRotationX);
		ImGui::SameLine();
		ImGui::Checkbox("Y##FreezeRotY", &rb->freezeRotationY);
		ImGui::SameLine();
		ImGui::Checkbox("Z##FreezeRotZ", &rb->freezeRotationZ);
	}

	void ImGuiLayer::DrawColliderFields(EntityID entity, Scene& scene)
	{
		ColliderComponent* col = scene.GetColliderComponent(entity);
		if (!col) return;

		// Shape dropdown
		const char* shapes[] = { _("Cube"), _("Sphere") };
		int currentShape = static_cast<int>(col->shape);
		if (ImGui::Combo(IMGUI_ELEMENT_TITLE("Shape", "ColliderShSape"), &currentShape, shapes, IM_ARRAYSIZE(shapes))) {
			col->shape = static_cast<ColliderShape>(currentShape);
		}

		// Shape-specific fields
		if (col->shape == ColliderShape::Box) {
			ImGui::DragFloat3(IMGUI_ELEMENT_TITLE("Half Extents", "Half Extents"), &col->boxHalfExtents.x, 0.05f, 0.01f, 1000.0f, "%.3f");
		}
		else if (col->shape == ColliderShape::Sphere) {
			ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Radius", "Radius"), &col->sphereRadius, 0.05f, 0.01f, 1000.0f, "%.3f");
		}

		ImGui::Checkbox(IMGUI_ELEMENT_TITLE("Is Trigger", "Is Trigger"), &col->isTrigger);
		ImGui::DragFloat3(IMGUI_ELEMENT_TITLE("Offset", "Collider Offset"), &col->offset.x, 0.05f, -1000.0f, 1000.0f, "%.3f");
	}

	void ImGuiLayer::DrawPointLightFields(EntityID entity, Scene& scene)
	{
		PointLightComponent* plc = scene.GetPointLightComponent(entity);
		if (!plc) return;

		ImGui::ColorEdit3("Light Color", &plc->color.x);
		ImGui::DragFloat("Intensity", &plc->intensity, 0.05f, 0.0f, 100.0f, "%.2f");
		ImGui::DragFloat("Constant", &plc->constant, 0.01f, 0.001f, 10.0f, "%.3f");
		ImGui::DragFloat("Linear", &plc->linear, 0.005f, 0.0f, 2.0f, "%.4f");
		ImGui::DragFloat("Quadratic", &plc->quadratic, 0.005f, 0.0f, 2.0f, "%.4f");
	}

	void ImGuiLayer::DrawAudioSourceFields(EntityID entity, Scene& scene)
	{
		AudioSourceComponent* asc = scene.GetAudioSourceComponent(entity);
		if (!asc) return;

		// Clip path: editable text or dropdown of known audio clips
		char pathBuf[512];
		strncpy_s(pathBuf, asc->clipPath.c_str(), sizeof(pathBuf) - 1);
		pathBuf[sizeof(pathBuf) - 1] = '\0';
		if (ImGui::InputText(IMGUI_ELEMENT_TITLE("Clip Path", "AudioClipPath"), pathBuf, sizeof(pathBuf)))
			asc->clipPath = pathBuf;

		// Dropdown of all registered audio clips for easy selection
		std::string previewClip = asc->clipPath.empty() ? "(none)" : std::filesystem::path(asc->clipPath).filename().string();
		if (ImGui::BeginCombo(IMGUI_ELEMENT_TITLE("Clip", "AudioClipCombo"), previewClip.c_str()))
		{
			if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("(none)", "AudioClipNone"), asc->clipPath.empty()))
				asc->clipPath.clear();

			for (auto& [id, asset] : AssetManager::GetAllAudioClipAssets())
			{
				// Show just the asset name; store the scene-ref in the component
				std::string sceneRef = AssetManager::ToSceneRef(asset.filePath);

				bool selected = (asc->clipPath == sceneRef || asc->clipPath == asset.filePath);
				if (ImGui::Selectable(asset.name.c_str(), selected))
					asc->clipPath = sceneRef;

				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();
		ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Volume", "AudioVolume"), &asc->volume, 0.01f, 0.0f, 5.0f, "%.2f");
		ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Pitch",  "AudioPitch"),  &asc->pitch,  0.01f, 0.1f, 4.0f, "%.2f");
		ImGui::Checkbox (IMGUI_ELEMENT_TITLE("Loop",         "AudioLoop"),        &asc->loop);
		ImGui::Checkbox (IMGUI_ELEMENT_TITLE("Play On Start","AudioPlayOnStart"), &asc->playOnStart);
		ImGui::Checkbox (IMGUI_ELEMENT_TITLE("Spatial (3D)", "AudioSpatial"),     &asc->spatial);

		if (asc->spatial) {
			ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Min Distance", "AudioMinDist"), &asc->minDistance, 0.1f, 0.01f, 1000.0f, "%.2f");
			ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Max Distance", "AudioMaxDist"), &asc->maxDistance, 0.5f, 0.1f,  5000.0f, "%.2f");
		}
	}

	void ImGuiLayer::DrawAudioListenerFields(EntityID /*entity*/, Scene& /*scene*/)
	{
		// The AudioListenerComponent has no configurable properties = its presence alone
		// tells the AudioEngine which entity is the listener.
		ImGui::TextDisabled("This entity receives audio. One listener per scene.");
	}

	// ---------- Draw one component entry ----------

	// Helper: draw a button that appears grayed-out and does nothing when disabled.
	// Works with any ImGui version (no BeginDisabled needed).
	static bool DisableableButton(const char* label, ImVec2 size, bool enabled)
	{
		if (!enabled) {
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.3f);
			ImGui::Button(label, size);
			ImGui::PopStyleVar();
			return false;
		}
		return ImGui::Button(label, size);
	}

	bool ImGuiLayer::DrawComponent(ComponentType type, int index, EntityID entity, Scene& scene)
	{
		auto& order = m_ComponentOrder[entity];
		bool removed = false;

		// Unique ImGui ID per component slot so headers don't collide
		ImGui::PushID(index);

		// --- Header row: collapsing header + action buttons on the right ---
		float contentWidth = ImGui::GetContentRegionAvail().x;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen
			| ImGuiTreeNodeFlags_Framed
			| ImGuiTreeNodeFlags_AllowOverlap
			| ImGuiTreeNodeFlags_FramePadding;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
		float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
		const char* componentTypeName = ComponentTypeName(type);
		bool open = ImGui::TreeNodeEx(IMGUI_ELEMENT_TITLE(componentTypeName, componentTypeName), flags);
		ImGui::PopStyleVar();

		// Right-aligned buttons: [^] [v] [X]
		bool isRequired = IsRequiredComponent(type);
		int buttonCount = isRequired ? 2 : 3;
		float buttonWidth = 25.0f;
		float buttonsWidth = buttonCount * (buttonWidth + ImGui::GetStyle().ItemSpacing.x);

		ImGui::SameLine(contentWidth - buttonsWidth);

		// Move Up (grayed out if already first)
		if (DisableableButton("^", ImVec2(buttonWidth, lineHeight), index > 0)) {
			std::swap(order[index], order[index - 1]);
		}

		ImGui::SameLine();

		// Move Down (grayed out if already last)
		if (DisableableButton("v", ImVec2(buttonWidth, lineHeight), index < (int)order.size() - 1)) {
			std::swap(order[index], order[index + 1]);
		}

		// Remove button — only for non-required components
		if (!isRequired) {
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));

			if (ImGui::Button("X", ImVec2(buttonWidth, lineHeight))) {
				removed = true;
			}

			ImGui::PopStyleColor(3);
		}

		// Draw component fields if the header is expanded
		if (open) {
			switch (type) {
			case ComponentType::EntityData: DrawEntityDataFields(entity, scene);  break;
			case ComponentType::Transform:  DrawTransformFields(entity, scene);   break;
			case ComponentType::Mesh:       DrawMeshFields(entity, scene);        break;
			case ComponentType::Material:   DrawMaterialFields(entity, scene);    break;
			case ComponentType::Camera:     DrawCameraFields(entity, scene);      break;
			case ComponentType::Script:     DrawScriptFields(entity, scene);      break;
			case ComponentType::Rigidbody:     DrawRigidbodyFields(entity, scene);    break;
			case ComponentType::Collider:      DrawColliderFields(entity, scene);     break;
			case ComponentType::PointLight:    DrawPointLightFields(entity, scene);   break;
			case ComponentType::AudioSource:   DrawAudioSourceFields(entity, scene);  break;
			case ComponentType::AudioListener: DrawAudioListenerFields(entity, scene); break;
			}
			ImGui::TreePop();
		}

		ImGui::PopID();

		// Handle removal after drawing (so ImGui IDs stay stable this frame)
		if (removed) {
			switch (type) {
			case ComponentType::Mesh:     scene.RemoveMeshComponent(entity);     break;
			case ComponentType::Material: scene.RemoveMaterialComponent(entity);  break;
			case ComponentType::Camera:    scene.RemoveCameraComponent(entity);    break;
			case ComponentType::Script:    scene.RemoveScriptComponent(entity);    break;
			case ComponentType::Rigidbody:     scene.RemoveRigidbodyComponent(entity);    break;
			case ComponentType::Collider:      scene.RemoveColliderComponent(entity);     break;
			case ComponentType::PointLight:    scene.RemovePointLightComponent(entity);   break;
			case ComponentType::AudioSource:   scene.RemoveAudioSourceComponent(entity);  break;
			case ComponentType::AudioListener: scene.RemoveAudioListenerComponent(entity); break;
			default: break;
			}
			order.erase(order.begin() + index);
		}

		return removed;
	}

	// ---------- Add Component popup ----------

	void ImGuiLayer::DrawAddComponentPopup(EntityID entity, Scene& scene)
	{
		if (ImGui::BeginPopup("AddComponentPopup")) {
			ImGui::Text(_("Add Component"));
			ImGui::Separator();

			auto& order = m_ComponentOrder[entity];

			// Helper: checks if a type is already in the display order
			auto hasType = [&](ComponentType t) {
				return std::find(order.begin(), order.end(), t) != order.end();
				};

			int addableCount = 0;

			if (!hasType(ComponentType::Mesh)) {
				addableCount++;
				if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("Mesh", "AddMeshComp"))) {
					scene.AddMeshComponent(entity, MeshComponent{});
					order.push_back(ComponentType::Mesh);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Material)) {
				addableCount++;
				if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("Material", "AddMaterialComp"))) {
					scene.AddMaterialComponent(entity, MaterialComponent{});
					order.push_back(ComponentType::Material);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Camera)) {
				addableCount++;
				if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("Camera", "AddCamComp"))) {
					scene.AddCameraComponent(entity, CameraComponent{});
					order.push_back(ComponentType::Camera);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Script)) {
				addableCount++;
				if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("Script", "AddScriptComp"))) {
					scene.AddScriptComponent(entity, ScriptComponent{});
					order.push_back(ComponentType::Script);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Rigidbody)) {
				addableCount++;
				if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("Rigidbody", "AddRigidbodyComp"))) {
					scene.AddRigidbodyComponent(entity, RigidbodyComponent{});
					order.push_back(ComponentType::Rigidbody);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::Collider)) {
				addableCount++;
				if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("Collider", "AddColliderComp"))) {
					scene.AddColliderComponent(entity, ColliderComponent{});
					order.push_back(ComponentType::Collider);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::PointLight)) {
				addableCount++;
				if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("Point Light", "AddPointLightComp"))) {
					scene.AddPointLightComponent(entity, PointLightComponent{});
					order.push_back(ComponentType::PointLight);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::AudioSource)) {
				addableCount++;
				if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("Audio Source", "AddAudioSourceComp"))) {
					scene.AddAudioSourceComponent(entity, AudioSourceComponent{});
					order.push_back(ComponentType::AudioSource);
					ImGui::CloseCurrentPopup();
				}
			}

			if (!hasType(ComponentType::AudioListener)) {
				addableCount++;
				if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("Audio Listener", "AddAudioListenerComp"))) {
					scene.AddAudioListenerComponent(entity, AudioListenerComponent{});
					order.push_back(ComponentType::AudioListener);
					ImGui::CloseCurrentPopup();
				}
			}

			if (addableCount == 0) {
				ImGui::TextDisabled(IMGUI_ELEMENT_TITLE("All components added", "AllCompsAdded"));
			}

			ImGui::EndPopup();
		}
	}

	// ---------- Main Inspector module ----------

	void ImGuiLayer::ShowInspectorModule()
	{
		if (showInspectorModule)
		{
			if (ImGui::Begin(IMGUI_ELEMENT_TITLE("Inspector", "Inspector"), &showInspectorModule))
			{
				EntityID selected = EditorLayer::GetSelectedEntity();
				auto scene = SceneManager::GetActiveScene();

				// Entity selection takes priority — clear asset selection
				if (selected != INVALID_ENTITY && scene && scene->IsValidEntity(selected))
				{
					m_SelectedAssetPath.clear();
					// Rebuild the component display order when selection changes
					if (m_InspectorEntity != selected) {
						ImGui::ClearActiveID();   // abandon any in-progress widget edit on the old entity
						m_InspectorEntity = selected;
						m_ScaleUniform = false;   // don't leak toggle state across entities

						// Validate the cached order, if one exists.
						// A stale cache occurs when entity IDs are reused across scene loads,
						// or when components are added to an entity (e.g. after duplication)
						// before the inspector has had a chance to observe them.
						// Rule: if the entity owns a component that is absent from the cache,
						// discard the cache and rebuild from scratch.
						// User-defined reordering is preserved in all other cases.
						bool needsRebuild = true;
						auto cacheIt = m_ComponentOrder.find(selected);
						if (cacheIt != m_ComponentOrder.end()) {
							const auto& cached = cacheIt->second;
							auto inCache = [&](ComponentType t) {
								return std::find(cached.begin(), cached.end(), t) != cached.end();
							};
							needsRebuild =
								(scene->HasMeshComponent(selected)          && !inCache(ComponentType::Mesh))         ||
								(scene->HasMaterialComponent(selected)      && !inCache(ComponentType::Material))     ||
								(scene->HasCameraComponent(selected)        && !inCache(ComponentType::Camera))       ||
								(scene->HasScriptComponent(selected)        && !inCache(ComponentType::Script))       ||
								(scene->HasRigidbodyComponent(selected)     && !inCache(ComponentType::Rigidbody))    ||
								(scene->HasColliderComponent(selected)      && !inCache(ComponentType::Collider))     ||
								(scene->HasPointLightComponent(selected)    && !inCache(ComponentType::PointLight))   ||
								(scene->HasAudioSourceComponent(selected)   && !inCache(ComponentType::AudioSource))  ||
								(scene->HasAudioListenerComponent(selected) && !inCache(ComponentType::AudioListener));
						}
						if (needsRebuild)
							RebuildComponentOrder(selected, *scene);
					}
					std::string entID = _("Entity ID: ") + std::format("{}", selected);
					ImGui::Text(entID.c_str(), std::format("Entity ID: {}", selected));
					ImGui::Separator();

					// Draw each component in the user-defined display order
					auto& order = m_ComponentOrder[selected];
					for (int i = 0; i < (int)order.size(); i++) {
						// DrawComponent returns true if the component was removed,
						// which shifts indices — decrement i to recheck this slot.
						if (DrawComponent(order[i], i, selected, *scene)) {
							i--;
						}
					}

					ImGui::Separator();

					// Centered "Add Component" button
					float buttonWidth = 200.0f;
					float windowWidth = ImGui::GetContentRegionAvail().x;
					float cursorX = (windowWidth - buttonWidth) * 0.5f;
					if (cursorX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cursorX);

					if (ImGui::Button(IMGUI_ELEMENT_TITLE("Add Component", "InspectorAddComp"), ImVec2(buttonWidth, 0))) {
						ImGui::OpenPopup("AddComponentPopup");
					}

					DrawAddComponentPopup(selected, *scene);
				}
				else if (!m_SelectedAssetPath.empty())
				{
					// Show asset inspector based on selected file type
					std::filesystem::path selPath(m_SelectedAssetPath);
					std::string selExt = selPath.extension().string();
					for (char& c : selExt) c = (char)std::tolower((unsigned char)c);

					if (selExt == ".mtl") {
						ImGui::Text("Material: %s", selPath.filename().string().c_str());
						ImGui::Separator();

						// Collect matching material IDs first (avoid iterating const map while editing)
						std::vector<AssetID> matchingMats;
						for (auto& [id, asset] : AssetManager::GetAllMaterialAssets()) {
							if (asset.filePath.find(m_SelectedAssetPath + "::") == 0
								|| asset.filePath == m_SelectedAssetPath)
								matchingMats.push_back(id);
						}

						for (AssetID id : matchingMats) {
							MaterialAsset* asset = AssetManager::GetMaterialAsset(id);
							if (!asset) continue;

							if (matchingMats.size() > 1)
								ImGui::Text("%s", asset->name.c_str());

							auto mat = AssetManager::GetMaterial(id);

							ImGui::PushID((int)id);

							// Color tint
							glm::vec4 color = mat ? mat->GetColor() : asset->colorTint;
							if (ImGui::ColorEdit4(IMGUI_ELEMENT_TITLE("Color", "MatColor"), &color.x)) {
								asset->colorTint = color;
								if (mat) mat->SetColor(color);
							}

							// Specular color
							glm::vec3 spec = mat ? mat->GetSpecularColor() : asset->specularColor;
							if (ImGui::ColorEdit3(IMGUI_ELEMENT_TITLE("Specular", "MatSpec"), &spec.x)) {
								asset->specularColor = spec;
								if (mat) mat->SetSpecularColor(spec);
							}

							// Shininess
							if (ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Shininess", "MatShine"), &asset->specularShininess, 0.5f, 0.0f, 512.0f)) {
								if (mat) mat->SetShininess(asset->specularShininess);
							}

							// Transparency
							if (ImGui::Checkbox(IMGUI_ELEMENT_TITLE("Transparent", "MatTransparent"), &asset->isTransparent)) {
								if (mat) mat->SetTransparent(asset->isTransparent);
							}

							// Texture picker
							{
								auto currentTex = mat ? mat->GetDiffuseTexture() : nullptr;
								std::string currentName = currentTex ? std::filesystem::path(currentTex->GetPath()).filename().string() : _("None");

								if (ImGui::BeginCombo(IMGUI_ELEMENT_TITLE("Texture", "MatTexture"), currentName.c_str())) {
									// "None" option to clear the texture
									bool isNone = (currentTex == nullptr);
									if (ImGui::Selectable(IMGUI_ELEMENT_TITLE("None", "MatTextureNone"), isNone)) {
										if (mat) mat->SetDiffuseTexture(nullptr);
										asset->diffuseTexture = TextureAsset{}; // clear
									}

									// List all loaded textures
									for (auto& [texID, texAsset] : AssetManager::GetAllTextureAssets()) {
										bool isSelected = (currentTex && currentTex->GetPath() == texAsset.filePath);
										if (ImGui::Selectable(texAsset.name.c_str(), isSelected)) {
											auto newTex = AssetManager::GetTexture(texID);
											if (mat && newTex) mat->SetDiffuseTexture(newTex);
											asset->diffuseTexture = texAsset;
										}
									}
									ImGui::EndCombo();
								}

								if (currentTex) {
									std::string texSize = _("Size") + std::format(":{}{}", currentTex->GetWidth(), currentTex->GetHeight());
									ImGui::Text(texSize.c_str());
								}
							}

							ImGui::PopID();
							ImGui::Separator();
						}
					}
					else {
						ImGui::TextDisabled(_("No entity selected"));
					}
				}
				else
				{
					ImGui::TextDisabled(_("No entity selected"));
				}
			}
			ImGui::End();
		}
	}

	GLuint viewportTexture;
	int viewportWidth = 800;
	int viewportHeight = 600;

	void ImGuiLayer::ShowViewportModule()
	{
		if (!showViewportModule)
			return;

		// Zero window padding so the toolbar/image sit flush against the panel edges.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		bool windowOpen = ImGui::Begin(IMGUI_ELEMENT_TITLE("Viewport", "Viewport"), &showViewportModule);
		ImGui::PopStyleVar();

		if (windowOpen)
		{
			s_ViewportHovered = ImGui::IsWindowHovered();
			s_ViewportFocused = ImGui::IsWindowFocused();

			ImDrawList* drawList   = ImGui::GetWindowDrawList();
			float       contentW   = ImGui::GetContentRegionAvail().x;

			// ================================================================
			// Viewport Toolbar
			// ================================================================
			const float  tbH      = 28.0f;
			const float  btnPadX  =  6.0f;  // padding between toolbar edge and first button
			ImVec2       tbMin    = ImGui::GetCursorScreenPos();
			ImVec2       tbMax    = ImVec2(tbMin.x + contentW, tbMin.y + tbH);

			// Toolbar background + bottom border
			drawList->AddRectFilled(tbMin, tbMax, IM_COL32(28, 28, 28, 235));
			drawList->AddLine(ImVec2(tbMin.x, tbMax.y - 1), ImVec2(tbMax.x, tbMax.y - 1), IM_COL32(55, 55, 55, 255));

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 3.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(4.0f, 0.0f));

			// Vertically centre all widgets in the toolbar
			float btnH = ImGui::GetFrameHeight();
			float btnY = tbMin.y + (tbH - btnH) * 0.5f;

			// ---- Left group: Lit / Wireframe ----
			ImGui::SetCursorScreenPos(ImVec2(tbMin.x + btnPadX, btnY));
			ViewMode currentMode = Renderer::GetViewMode();

			auto viewBtn = [&](const char* label, ViewMode mode) {
				bool active = (currentMode == mode);
				ImVec4 col = active
					? ImVec4(0.26f, 0.59f, 0.98f, 0.90f)
					: ImVec4(0.18f, 0.18f, 0.18f, 0.85f);
				ImGui::PushStyleColor(ImGuiCol_Button,        col);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active
					? ImVec4(0.26f, 0.59f, 0.98f, 1.00f)
					: ImVec4(0.28f, 0.28f, 0.28f, 1.00f));
				if (ImGui::Button(label))
					Renderer::SetViewMode(mode);
				ImGui::PopStyleColor(2);
			};
			viewBtn(IMGUI_ELEMENT_TITLE("Lit",       "LitBtn"),       ViewMode::Lit);
			ImGui::SameLine();
			viewBtn(IMGUI_ELEMENT_TITLE("Wireframe", "WireframeBtn"), ViewMode::Wireframe);

			// ---- Centre group: Play / Stop ----
			bool  playing   = EditorLayer::IsPlaying();
			float playBtnW  = 54.0f;
			float groupW    = playBtnW * 2.0f + 4.0f;  // two buttons + 4 px gap
			float centerX   = tbMin.x + (contentW - groupW) * 0.5f;
			ImGui::SetCursorScreenPos(ImVec2(centerX, btnY));

			ImGui::PushStyleColor(ImGuiCol_Button,
				playing ? ImVec4(0.18f, 0.55f, 0.18f, 1.00f)
				        : ImVec4(0.22f, 0.48f, 0.22f, 0.80f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 0.25f, 1.0f));
			if (ImGui::Button(_("Play"), ImVec2(playBtnW, 0.0f)) && !playing)
				EditorLayer::RequestEnterPlay();
			ImGui::PopStyleColor(2);

			ImGui::SameLine(0.0f, 4.0f);

			ImGui::PushStyleColor(ImGuiCol_Button,
				playing ? ImVec4(0.65f, 0.18f, 0.18f, 1.00f)
				        : ImVec4(0.30f, 0.15f, 0.15f, 0.50f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.22f, 0.22f, 1.0f));
			if (ImGui::Button(_("Stop"), ImVec2(playBtnW, 0.0f)) && playing)
				EditorLayer::RequestExitPlay();
			ImGui::PopStyleColor(2);

			// ---- Right: FPS counter ----
			std::string fpsText = std::format("FPS: {:.0f}", ImGui::GetIO().Framerate);
			ImVec2 fpsSize = ImGui::CalcTextSize(fpsText.c_str());
			ImVec2 fpsPos  = ImVec2(tbMax.x - fpsSize.x - 10.0f,
			                        tbMin.y + (tbH - fpsSize.y) * 0.5f);
			drawList->AddText(fpsPos, IM_COL32(170, 170, 170, 255), fpsText.c_str());

			ImGui::PopStyleVar(2);

			// Advance cursor to just below the toolbar so the image starts there.
			ImGui::SetCursorScreenPos(ImVec2(tbMin.x, tbMax.y));

			// ================================================================
			// Framebuffer Image
			// ================================================================
			ImVec2 imageSize = ImGui::GetContentRegionAvail();
			if (imageSize.x < 1.0f) imageSize.x = 1.0f;
			if (imageSize.y < 1.0f) imageSize.y = 1.0f;

			// Resize the framebuffer to match the actual image area (excludes toolbar).
			Renderer::GetViewportFramebuffer()->Resize(
				static_cast<unsigned int>(imageSize.x),
				static_cast<unsigned int>(imageSize.y)
			);

			// Record image rect for picking and overlay positioning.
			s_ViewportImageMin = ImGui::GetCursorScreenPos();

			// UVs flipped vertically: OpenGL origin is bottom-left; ImGui expects top-left.
			ImGui::Image(
				(ImTextureID)(intptr_t)Renderer::GetViewportFramebuffer()->GetColorAttachment(),
				imageSize,
				ImVec2(0, 1),
				ImVec2(1, 0)
			);

			s_ViewportImageMax = ImVec2(
				s_ViewportImageMin.x + imageSize.x,
				s_ViewportImageMin.y + imageSize.y
			);

			// ---- Toast notification (bottom-centre of rendered image) ----
			if (s_NotificationTimer > 0.0f) {
				s_NotificationTimer -= ImGui::GetIO().DeltaTime;

				float alpha  = glm::clamp(s_NotificationTimer / 0.5f, 0.0f, 1.0f);
				ImU32 bgCol  = IM_COL32(30,  30,  30,  static_cast<int>(220 * alpha));
				ImU32 txtCol = IM_COL32(255, 255, 255, static_cast<int>(255 * alpha));

				ImVec2 textSize  = ImGui::CalcTextSize(s_NotificationMessage.c_str());
				float  pad       = 10.0f;
				float  vpCentreX = (s_ViewportImageMin.x + s_ViewportImageMax.x) * 0.5f;
				float  y         = s_ViewportImageMax.y - textSize.y - pad * 3.0f;
				ImVec2 bgMin = ImVec2(vpCentreX - textSize.x * 0.5f - pad, y - pad);
				ImVec2 bgMax = ImVec2(vpCentreX + textSize.x * 0.5f + pad, y + textSize.y + pad);

				drawList->AddRectFilled(bgMin, bgMax, bgCol, 4.0f);
				drawList->AddText(ImVec2(bgMin.x + pad, y), txtCol, s_NotificationMessage.c_str());
			}
		}

		ImGui::End();
	}

	// ================================================================
	// Hierarchy Panel — shows all entities as a tree using ECS data
	// ================================================================

	void ImGuiLayer::ShowHierarchyModule()
	{
		if (!showHierarchyModule) { return; }

		if (ImGui::Begin(IMGUI_ELEMENT_TITLE("Hierarchy", "Hierarchy"), &showHierarchyModule))
		{
			auto scene = SceneManager::GetActiveScene();
			if (scene)
			{
				// Only draw root entities (those with no parent).
				// Children are drawn recursively inside DrawEntityNode.
				std::vector<EntityID> roots = scene->GetRootEntities();
				for (EntityID root : roots)
				{
					DrawEntityNode(root, *scene);
				}

				// Right-click on empty space = context menu for the whole panel
				if (ImGui::BeginPopupContextWindow("HierarchyContextMenu",
					ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
					if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Create Empty Entity", "HierCreateEmptyEntity"))) {
						EntityID newEntity = scene->CreateEntity();
						scene->AddEntityDataComponent(newEntity, EntityDataComponent{});
						scene->AddTransformComponent(newEntity, TransformComponent{});
						EditorLayer::SetSelectedEntity(newEntity);
					}

					if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Create Cube",        "HierCreateCube")))
						EditorLayer::AddPrimitive("Cube", "cube");
					if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Create Sphere",      "HierCreateSphere")))
						EditorLayer::AddPrimitive("Sphere", "sphere");
					if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Create Plane",       "HierCreatePlane")))
						EditorLayer::AddPrimitive("Plane", "square");
					ImGui::Separator();
					if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Create Point Light", "HierCreatePointLight")))
						EditorLayer::AddPointLight();

					ImGui::EndPopup();

					// Drop on empty space = unparent (make root)
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"))
						{
							EntityID droppedEntity = *(const EntityID*)payload->Data;
							scene->RemoveParent(droppedEntity);
						}
						ImGui::EndDragDropTarget();
					}
				}
			}
			ImGui::End();
		}
	}

	void ImGuiLayer::DrawEntityNode(EntityID entity, Scene& scene)
	{
		// Get the entity's display name.
		EntityDataComponent* edc = scene.GetEntityDataComponent(entity);
		std::string name = edc ? edc->name : ("Entity " + std::to_string(entity));

		// Is this entity currently selected?
		bool isSelected = (entity == EditorLayer::GetSelectedEntity());

		// Does this entity have children?
		bool hasChildren = scene.HasChildren(entity);

		// Build tree node flags:
		//   - OpenOnArrow: only expand when clicking the arrow, not the label
		//   - SpanAvailWidth: the node spans the full width (easier to click)
		//   - Selected: highlight if this is the selected entity
		//   - Leaf: no arrow if no children
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		if (isSelected)
			flags |= ImGuiTreeNodeFlags_Selected;

		if (!hasChildren)
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		// Use the entity ID as the ImGui ID to keep nodes unique.
		bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entity, flags, "%s", name.c_str());

		// --- Click to select ---
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			EditorLayer::SetSelectedEntity(entity);
		}

		// --- Drag source: start dragging this entity ---
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &entity, sizeof(EntityID));
			ImGui::Text("%s", name.c_str());
			ImGui::EndDragDropSource();
		}

		// --- Drop target: reparent the dragged entity under this one ---
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"))
			{
				EntityID droppedEntity = *(const EntityID*)payload->Data;
				// Don't parent an entity to itself.
				if (droppedEntity != entity)
				{
					scene.SetParent(droppedEntity, entity);
				}
			}
			ImGui::EndDragDropTarget();
		}

		// --- Right-click context menu on this entity ---
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Duplicate", "HierDuplicateEntity")))
			{
				EntityID copy = scene.DuplicateEntity(entity);
				if (copy != INVALID_ENTITY)
					EditorLayer::SetSelectedEntity(copy);
			}

			if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Create Child Entity", "HierCreateChildEntity")))
			{
				EntityID child = scene.CreateEntity();
				scene.AddEntityDataComponent(child, EntityDataComponent{});
				scene.AddTransformComponent(child, TransformComponent{});
				scene.SetParent(child, entity);
				EditorLayer::SetSelectedEntity(child);
			}

			if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Delete Entity", "HierDeleteChildEntity")))
			{
				// If this was selected, clear selection.
				if (EditorLayer::GetSelectedEntity() == entity)
					EditorLayer::SetSelectedEntity(INVALID_ENTITY);

				scene.DestroyEntity(entity);

				ImGui::EndPopup();
				// Node was destroyed — don't draw children.
				if (nodeOpen && hasChildren)
					ImGui::TreePop();
				return;
			}

			if (scene.HasParent(entity))
			{
				if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Unparent", "Unparent")))
				{
					scene.RemoveParent(entity);
				}
			}

			ImGui::EndPopup();
		}

		// --- Recursively draw children if the node is open ---
		if (nodeOpen && hasChildren)
		{
			const auto& children = scene.GetChildren(entity);
			for (EntityID child : children)
			{
				DrawEntityNode(child, scene);
			}
			ImGui::TreePop();
		}
	}

	// ========================== ASSET BROWSER ==========================

	namespace fs = std::filesystem;

	// Returns a label prefix based on file extension for visual identification.
	static const char* FileTypeIcon(const std::string& ext)
	{
		if (ext == ".obj")   return "[Mesh]    ";
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
			return "[Tex]     ";
		if (ext == ".mtl")   return "[Mat]     ";
		if (ext == ".lua")   return "[Script]  ";
		if (ext == ".scene") return "[Scene]   ";
		return "          ";
	}

	// Returns a color for each file type so entries are visually distinct.
	static ImVec4 FileTypeColor(const std::string& ext)
	{
		if (ext == ".obj")   return ImVec4(0.4f, 0.8f, 1.0f, 1.0f);  // light blue
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
			return ImVec4(0.9f, 0.7f, 0.3f, 1.0f);  // amber
		if (ext == ".mtl")   return ImVec4(0.5f, 1.0f, 0.5f, 1.0f);  // green
		if (ext == ".lua")   return ImVec4(0.6f, 0.6f, 1.0f, 1.0f);  // purple-ish
		if (ext == ".scene") return ImVec4(1.0f, 0.6f, 0.6f, 1.0f);  // salmon
		return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // grey
	}

	void ImGuiLayer::ShowAssetBrowser()
	{
		if (!showFileDirectoryModule)
			return;

		if (ImGui::Begin(IMGUI_ELEMENT_TITLE("Assets", "Assets"), &showFileDirectoryModule))
		{
			std::string assetsRoot = AssetManager::GetAssetsFolderPath();
			if (assetsRoot.empty()) {
				ImGui::TextDisabled(IMGUI_ELEMENT_TITLE("Assets folder path not set.", "AssetsFolderPathNone"));
				ImGui::End();
				return;
			}

			// --- Tab bar: Project assets / Engine assets ---
			static int s_AssetBrowserTab = 0;  // 0 = Project, 1 = Engine
			bool hasEngineAssets = !AssetManager::GetEngineAssetsFolderPath().empty() &&
			                       fs::exists(AssetManager::GetEngineAssetsFolderPath());

			if (hasEngineAssets)
			{
				bool switchedToProject = false;
				bool switchedToEngine  = false;

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 4));
				if (ImGui::BeginTabBar("AssetBrowserTabs"))
				{
					if (ImGui::BeginTabItem(IMGUI_ELEMENT_TITLE("Project", "ABTabProject"))) {
						if (s_AssetBrowserTab != 0) { s_AssetBrowserTab = 0; switchedToProject = true; }
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(IMGUI_ELEMENT_TITLE("Engine", "ABTabEngine"))) {
						if (s_AssetBrowserTab != 1) { s_AssetBrowserTab = 1; switchedToEngine = true; }
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::PopStyleVar();

				// When switching tabs, reset current dir to the new root
				if (switchedToProject)
					m_AssetBrowserCurrentDir = fs::path(assetsRoot).make_preferred().string();
				else if (switchedToEngine)
					m_AssetBrowserCurrentDir = fs::path(AssetManager::GetEngineAssetsFolderPath()).make_preferred().string();
			}

			// Determine which root to browse based on active tab
			bool browsingEngineAssets = (s_AssetBrowserTab == 1 && hasEngineAssets);
			std::string activeRoot = browsingEngineAssets
			    ? AssetManager::GetEngineAssetsFolderPath()
			    : assetsRoot;

			// Ensure root path is absolute with consistent separators so all paths
			// derived from it (m_AssetBrowserCurrentDir, m_SelectedAssetPath) match
			// the absolute keys stored by AssetManager after normalization.
			fs::path rootPath = fs::absolute(fs::path(activeRoot)).make_preferred();

			// Initialise current directory to root on first open
			if (m_AssetBrowserCurrentDir.empty())
				m_AssetBrowserCurrentDir = rootPath.string();

			// If current dir is not under the active root, reset it
			{
				std::string curNorm  = fs::path(m_AssetBrowserCurrentDir).make_preferred().string();
				std::string rootNorm = rootPath.string();
				if (curNorm.find(rootNorm) != 0)
					m_AssetBrowserCurrentDir = rootNorm;
			}

			fs::path currentPath(m_AssetBrowserCurrentDir);
			if (!fs::exists(currentPath) || !fs::is_directory(currentPath))
				currentPath = rootPath;

			// ---------- Breadcrumb navigation ----------
			{
				// Build path segments from root to current
				fs::path relative = fs::relative(currentPath, rootPath);
				std::vector<std::pair<std::string, fs::path>> crumbs;

				// Root crumb
				crumbs.push_back({ browsingEngineAssets ? "Engine" : "Assets", rootPath });

				// Walk the relative path and build intermediate absolute paths
				if (relative != "." && !relative.empty()) {
					fs::path building = rootPath;
					for (const auto& segment : relative) {
						building /= segment;
						crumbs.push_back({ segment.string(), building });
					}
				}

				for (size_t i = 0; i < crumbs.size(); ++i) {
					if (i > 0) {
						ImGui::SameLine(0, 2);
						ImGui::TextDisabled("/");
						ImGui::SameLine(0, 2);
					}

					bool isCurrent = (i == crumbs.size() - 1);
					if (isCurrent) {
						ImGui::TextUnformatted(crumbs[i].first.c_str());
					}
					else {
						// Clickable breadcrumb — navigate to that folder
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
						if (ImGui::SmallButton(crumbs[i].first.c_str())) {
							m_AssetBrowserCurrentDir = crumbs[i].second.string();
						}
						ImGui::PopStyleColor(2);
					}
				}
			}

			// Opens a native multi-select file dialog and copies chosen files into
			// the current browser directory, registering them with AssetManager.
			auto importAssets = [&]() {
#if defined(_WIN32)
				// Buffer large enough for many files: first entry is the directory,
				// subsequent entries are filenames (all null-separated, double-null terminated).
				static char fileBuffer[8192];
				ZeroMemory(fileBuffer, sizeof(fileBuffer));

				OPENFILENAMEA ofn   = {};
				ofn.lStructSize     = sizeof(ofn);
				ofn.lpstrFilter     =
					"All Supported Assets\0*.obj;*.png;*.jpg;*.jpeg;*.tga;*.bmp;*.mtl;*.wav;*.mp3;*.ogg;*.flac;*.lua;*.scene\0"
					"3D Models (*.obj)\0*.obj\0"
					"Textures (*.png;*.jpg;*.jpeg;*.tga;*.bmp)\0*.png;*.jpg;*.jpeg;*.tga;*.bmp\0"
					"Materials (*.mtl)\0*.mtl\0"
					"Audio (*.wav;*.mp3;*.ogg;*.flac)\0*.wav;*.mp3;*.ogg;*.flac\0"
					"Scripts (*.lua)\0*.lua\0"
					"Scenes (*.scene)\0*.scene\0"
					"All Files (*.*)\0*.*\0";
				ofn.lpstrFile       = fileBuffer;
				ofn.nMaxFile        = sizeof(fileBuffer);
				ofn.lpstrTitle      = "Import Asset(s)";
				ofn.lpstrInitialDir = currentPath.string().c_str();
				ofn.Flags           = OFN_EXPLORER | OFN_ALLOWMULTISELECT |
				                      OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

				if (!GetOpenFileNameA(&ofn))
					return;

				// Parse the result buffer.
				// Single selection: buffer = full path (no second null yet).
				// Multi selection:  buffer = "dir\0name1\0name2\0\0"
				std::vector<std::string> srcPaths;
				char* p = fileBuffer;
				std::string first = p;
				p += first.size() + 1;

				if (*p == '\0') {
					// Single file
					srcPaths.push_back(first);
				} else {
					// Multiple files — first token is the directory
					while (*p != '\0') {
						std::string fname = p;
						srcPaths.push_back(first + "\\" + fname);
						p += fname.size() + 1;
					}
				}

				// Copy and register each file
				std::string lastImported;
				for (const auto& srcStr : srcPaths) {
					fs::path src(srcStr);
					fs::path dest = currentPath / src.filename();

					// Skip if an identical file already exists (warn, don't overwrite silently)
					if (fs::exists(dest)) {
						AddConsoleMessage(ConsoleEntry::Level::Warning, "Import",
							("Skipped (already exists): " + src.filename().string()).c_str());
						continue;
					}

					try {
						fs::copy_file(src, dest);
					}
					catch (const std::exception& e) {
						AddConsoleMessage(ConsoleEntry::Level::Error, "Import",
							(std::string("Copy failed — ") + src.filename().string() + ": " + e.what()).c_str());
						continue;
					}

					// Register with AssetManager by extension
					std::string ext = dest.extension().string();
					for (char& c : ext) c = (char)std::tolower((unsigned char)c);

					if (ext == ".obj") {
						AssetManager::LoadMesh(dest.string());
					} else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
					           ext == ".tga" || ext == ".bmp") {
						AssetManager::LoadTexture(dest.string());
					} else if (ext == ".mtl") {
						AssetManager::LoadMTLFile(dest.string());
					} else if (ext == ".wav" || ext == ".mp3" ||
					           ext == ".ogg" || ext == ".flac") {
						AssetManager::LoadAudioClip(dest.string());
					}
					// .lua and .scene are used on demand — no pre-registration needed.

					AddConsoleMessage(ConsoleEntry::Level::Trace, "Import",
						("Imported: " + src.filename().string()).c_str());
					lastImported = AssetManager::NormalizePlainPath(dest.string());
				}

				// Select the last imported file so the inspector opens it immediately
				if (!lastImported.empty()) {
					m_SelectedAssetPath = lastImported;
					EditorLayer::SetSelectedEntity(INVALID_ENTITY);
				}
#endif
			};

			ImGui::Separator();

			// ---------- Search + Import toolbar ----------
			static ImGuiTextFilter filter;
			{
				// Reserve space for the Import button (project tab only); full-width otherwise.
				const float importBtnW = browsingEngineAssets ? 0.0f
				    : ImGui::CalcTextSize(_("Import Asset...")).x
				      + ImGui::GetStyle().FramePadding.x * 2.0f + 8.0f;
				float searchW = ImGui::GetContentRegionAvail().x - importBtnW
				                - (browsingEngineAssets ? 0.0f : ImGui::GetStyle().ItemSpacing.x);

				filter.Draw("##AssetFilter", searchW);

				if (!browsingEngineAssets) {
					ImGui::SameLine();
					if (ImGui::Button(_("Import Asset...")))
						importAssets();
				}
			}
			bool isSearching = filter.IsActive();

			ImGui::Separator();

			// ---------- Back button (go to parent) ----------
			if (!isSearching && currentPath != rootPath) {
				if (ImGui::Selectable("..  [Up]", false)) {
					fs::path parent = currentPath.parent_path();
					// Don't go above root
					if (parent.string().size() >= rootPath.string().size())
						m_AssetBrowserCurrentDir = parent.string();
					else
						m_AssetBrowserCurrentDir = rootPath.string();
				}
			}

			// ---------- Helpers: create new assets in the current directory ----------
			// These lambdas generate a unique filename (e.g. "New Scene (2).scene")
			// and write a minimal valid file so the asset system can pick it up.

			static bool s_ShowRenamePopup = false;
			static char s_RenameBuf[256] = "";
			static std::string s_RenameTargetPath;   // absolute path of the file/folder being renamed

			static bool        s_ShowDeleteConfirm = false;
			static std::string s_PendingDeletePath;
			static bool        s_PendingDeleteIsFolder = false;

			auto uniquePath = [&](const std::string& baseName, const std::string& ext) -> fs::path {
				fs::path candidate = currentPath / (baseName + ext);
				if (!fs::exists(candidate)) return candidate;
				for (int i = 2; i < 100; ++i) {
					candidate = currentPath / (baseName + " (" + std::to_string(i) + ")" + ext);
					if (!fs::exists(candidate)) return candidate;
				}
				return candidate;
				};

			auto createNewScene = [&]() {
				fs::path path = uniquePath("New Scene", ".scene");
				std::ofstream file(path);
				if (file.is_open()) {
					file << "{\n  \"scene\": {\n    \"name\": \"" << path.stem().string()
						<< "\",\n    \"version\": 1\n  },\n  \"entities\": []\n}\n";
					file.close();
					std::cout << "[Assets] Created: " << path.string() << "\n";
				}
				};

			auto createNewMaterial = [&]() {
				fs::path path = uniquePath("New Material", ".mtl");
				std::ofstream file(path);
				if (file.is_open()) {
					file << "# Wavefront Material\n"
						<< "newmtl " << path.stem().string() << "\n"
						<< "Kd 0.9 0.9 0.9\n"
						<< "Ks 0.5 0.5 0.5\n"
						<< "Ns 32.0\n"
						<< "d 1.0\n";
					file.close();
					AssetManager::LoadMTLFile(path.string());
					std::cout << "[Assets] Created: " << path.string() << "\n";
				}
				};

			auto createNewFolder = [&]() {
				fs::path candidate = currentPath / "New Folder";
				if (fs::exists(candidate)) {
					for (int i = 2; i < 100; ++i) {
						candidate = currentPath / ("New Folder (" + std::to_string(i) + ")");
						if (!fs::exists(candidate)) break;
					}
				}
				try {
					fs::create_directory(candidate);
					std::cout << "[Assets] Created folder: " << candidate.string() << "\n";
				}
				catch (const std::exception& e) {
					std::cout << "[Assets] Failed to create folder: " << e.what() << "\n";
				}
				};

			auto createNewScript = [&]() {
				fs::path path = uniquePath("New Script", ".lua");
				std::ofstream file(path);
				if (file.is_open()) {
					file << "-- Script: " << path.stem().string() << "\n\n"
						<< "function OnStart()\n"
						<< "    -- Called once when play mode begins\n"
						<< "end\n\n"
						<< "function OnUpdate(dt)\n"
						<< "    -- Called every frame with delta time\n"
						<< "end\n";
					file.close();
					std::cout << "[Assets] Created: " << path.string() << "\n";
				}
				};

			// ---------- Directory listing ----------
			if (isSearching) {
				// Recursive search across all assets
				try {
					for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
						if (!entry.is_regular_file())
							continue;

						std::string filename = entry.path().filename().string();
						if (!filter.PassFilter(filename.c_str()))
							continue;

						// Show relative path from assets root
						std::string relPath = fs::relative(entry.path(), rootPath).string();
						std::string ext = entry.path().extension().string();
						for (char& c : ext) c = (char)std::tolower((unsigned char)c);

						std::string label = std::string(FileTypeIcon(ext)) + relPath;

						ImGui::PushStyleColor(ImGuiCol_Text, FileTypeColor(ext));
						if (ImGui::Selectable(label.c_str(), false)) {
							// Navigate to the containing folder
							m_AssetBrowserCurrentDir = entry.path().parent_path().string();
							filter.Clear();
						}
						ImGui::PopStyleColor();
					}
				}
				catch (const std::exception&) { /* skip permission errors */ }
			}
			else {
				// Collect and sort entries: directories first, then files alphabetically
				std::vector<fs::directory_entry> dirs;
				std::vector<fs::directory_entry> files;

				try {
					for (const auto& entry : fs::directory_iterator(currentPath)) {
						if (entry.is_directory())
							dirs.push_back(entry);
						else if (entry.is_regular_file())
							files.push_back(entry);
					}
				}
				catch (const std::exception&) { /* skip */ }

				auto sortByName = [](const fs::directory_entry& a, const fs::directory_entry& b) {
					return a.path().filename().string() < b.path().filename().string();
					};
				std::sort(dirs.begin(), dirs.end(), sortByName);
				std::sort(files.begin(), files.end(), sortByName);

				// Folders
				for (const auto& dir : dirs) {
					std::string name = dir.path().filename().string();
					std::string label = std::string("[Folder]  ") + name;

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.5f, 1.0f)); // warm yellow
					if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
						if (ImGui::IsMouseDoubleClicked(0)) {
							m_AssetBrowserCurrentDir = dir.path().string();
						}
					}
					ImGui::PopStyleColor();

					// Right-click context menu for folders
					if (ImGui::BeginPopupContextItem()) {
						if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Open", "FolderOpen"))) {
							m_AssetBrowserCurrentDir = dir.path().string();
						}
						if (!browsingEngineAssets) {
							if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Rename", "FolderRename"))) {
								s_RenameTargetPath = dir.path().string();
								strncpy_s(s_RenameBuf, name.c_str(), sizeof(s_RenameBuf) - 1);
								s_ShowRenamePopup = true;
							}
						}
						ImGui::Separator();
						if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Show in Explorer", "FolderShowInExplorer"))) {
							std::string absPath = fs::absolute(dir.path()).string();
							ShellExecuteA(NULL, "explore", absPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
						}
						if (!browsingEngineAssets) {
							ImGui::Separator();
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
							if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Delete", "FolderDelete"))) {
								s_PendingDeletePath     = dir.path().string();
								s_PendingDeleteIsFolder = true;
								s_ShowDeleteConfirm     = true;
								ImGui::OpenPopup("DeleteConfirmPopup");
							}
							ImGui::PopStyleColor();
						}
						ImGui::EndPopup();
					}
				}

				// Files
				for (const auto& file : files) {
					std::string name = file.path().filename().string();
					std::string ext = file.path().extension().string();
					for (char& c : ext) c = (char)std::tolower((unsigned char)c);

					std::string label = std::string(FileTypeIcon(ext)) + name;

					bool isSelected = (m_SelectedAssetPath == file.path().string());
					ImGui::PushStyleColor(ImGuiCol_Text, FileTypeColor(ext));
					bool clicked = ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick);
					ImGui::PopStyleColor();

					// Single click selects the asset (for inspector editing).
					// Store as normalized absolute path so it matches AssetManager keys.
					if (clicked && !ImGui::IsMouseDoubleClicked(0)) {
						m_SelectedAssetPath = AssetManager::NormalizePlainPath(file.path().string());
						EditorLayer::SetSelectedEntity(INVALID_ENTITY);
					}

					// Right-click context menu for files
					if (ImGui::BeginPopupContextItem()) {
						if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Open", "FileOpen"))) {
							if (ext == ".scene") {
								SceneManager::LoadScene(file.path().string());
							}
							else {
								std::string absPath = fs::absolute(file.path()).string();
								ShellExecuteA(NULL, "open", absPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
							}
						}
						if (!browsingEngineAssets) {
							if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Rename", "FileRename"))) {
								s_RenameTargetPath = file.path().string();
								strncpy_s(s_RenameBuf, name.c_str(), sizeof(s_RenameBuf) - 1);
								s_ShowRenamePopup = true;
							}
						}
						ImGui::Separator();
						if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Show in Explorer", "FileShowInExplorer"))) {
							std::string absPath = fs::absolute(file.path().parent_path()).string();
							ShellExecuteA(NULL, "explore", absPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
						}
						if (!browsingEngineAssets) {
							ImGui::Separator();
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
							if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Delete", "FileDelete"))) {
								s_PendingDeletePath     = file.path().string();
								s_PendingDeleteIsFolder = false;
								s_ShowDeleteConfirm     = true;
								ImGui::OpenPopup("DeleteConfirmPopup");
							}
							ImGui::PopStyleColor();
						}
						ImGui::EndPopup();
					}

					// Tooltip with full relative path and file size
					if (ImGui::IsItemHovered()) {
						std::string relPath = fs::relative(file.path(), rootPath).string();
						auto fileSize = file.file_size();
						std::string sizeStr;
						if (fileSize < 1024)
							sizeStr = std::to_string(fileSize) + " B";
						else if (fileSize < 1024 * 1024)
							sizeStr = std::format("{:.1f} KB", fileSize / 1024.0);
						else
							sizeStr = std::format("{:.1f} MB", fileSize / (1024.0 * 1024.0));

						ImGui::BeginTooltip();
						//ImGui::Text("Path: %s", relPath.c_str());
						//ImGui::Text("Size: %s", sizeStr.c_str());
						std::string pathTooltip = _("Path") + std::format(": {}", relPath.c_str());
						std::string sizeTooltip = _("Size") + std::format(": {}", sizeStr.c_str());

						ImGui::Text(pathTooltip.c_str());
						ImGui::Text(sizeTooltip.c_str());
						ImGui::EndTooltip();
					}

					// Double-click opens the file with the OS-registered application
					if (clicked && ImGui::IsMouseDoubleClicked(0)) {
						if (ext == ".scene") {
							SceneManager::LoadScene(file.path().string());
						}
						else {
							std::string absPath = fs::absolute(file.path()).string();
							ShellExecuteA(NULL, "open", absPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
						}
					}
				}
			}

			// ---------- Right-click on empty space: "Create" context menu ----------
			if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
				if (!browsingEngineAssets) {
					if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Import Asset...", "CtxImportAsset")))
						importAssets();
					ImGui::Separator();
					if (ImGui::BeginMenu(IMGUI_ELEMENT_TITLE("New", "NewAsset"))) {
						if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Folder", "NewFolder")))
							createNewFolder();
						ImGui::Separator();
						if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Scene (.scene)", "NewScene")))
							createNewScene();
						if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Material (.mtl)", "NewMaterial")))
							createNewMaterial();
						if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Lua Script (.lua)", "NewScript")))
							createNewScript();
						ImGui::EndMenu();
					}
					ImGui::Separator();
				}
				if (ImGui::MenuItem(IMGUI_ELEMENT_TITLE("Show in Explorer", "ShowInExplorerAsset"))) {
					std::string absPath = fs::absolute(currentPath).string();
					ShellExecuteA(NULL, "explore", absPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
				}
				ImGui::EndPopup();
			}

			// ---------- Rename popup (modal) ----------
			if (s_ShowRenamePopup) {
				ImGui::OpenPopup(IMGUI_ELEMENT_TITLE("Rename", "AssetRename"));
				s_ShowRenamePopup = false;
			}

			if (ImGui::BeginPopupModal(IMGUI_ELEMENT_TITLE("Rename", "AssetRename"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text(_("Enter new name:"));
				bool enter = ImGui::InputText("##RenameInput", s_RenameBuf, sizeof(s_RenameBuf),
					ImGuiInputTextFlags_EnterReturnsTrue);

				// Auto-focus the text input on first frame
				if (ImGui::IsWindowAppearing())
					ImGui::SetKeyboardFocusHere(-1);

				if (ImGui::Button("OK", ImVec2(80, 0)) || enter) {
					std::string newName(s_RenameBuf);
					if (!newName.empty() && !s_RenameTargetPath.empty()) {
						fs::path oldPath(s_RenameTargetPath);
						fs::path newPath = oldPath.parent_path() / newName;
						try {
							fs::rename(oldPath, newPath);
							std::cout << "[Assets] Renamed: " << oldPath.filename().string()
								<< " -> " << newName << "\n";

							// If we renamed the current directory, update the path
							if (m_AssetBrowserCurrentDir == oldPath.string())
								m_AssetBrowserCurrentDir = AssetManager::NormalizePlainPath(newPath.string());

							// If we renamed a selected asset, update the selection
							if (m_SelectedAssetPath == AssetManager::NormalizePlainPath(oldPath.string()))
								m_SelectedAssetPath = AssetManager::NormalizePlainPath(newPath.string());

							// If it's a .mtl file, update newmtl tags and AssetManager keys
							std::string ext = newPath.extension().string();
							for (char& c : ext) c = (char)std::tolower((unsigned char)c);
							if (ext == ".mtl") {
								std::string newStem = newPath.stem().string();

								// Rewrite newmtl lines inside the file
								std::ifstream inFile(newPath);
								if (inFile.is_open()) {
									std::stringstream buf;
									std::string line;
									while (std::getline(inFile, line)) {
										if (line.rfind("newmtl ", 0) == 0)
											buf << "newmtl " << newStem << "\n";
										else
											buf << line << "\n";
									}
									inFile.close();
									std::ofstream outFile(newPath, std::ios::trunc);
									outFile << buf.str();
									outFile.close();
								}

								// Re-key materials in AssetManager from old path to new
								std::string oldPathStr = oldPath.string();
								std::string newPathStr = newPath.string();
								std::vector<std::pair<AssetID, std::string>> toRekey;
								for (auto& [id, asset] : AssetManager::GetAllMaterialAssets()) {
									if (asset.filePath.find(oldPathStr + "::") == 0)
										toRekey.push_back({ id, asset.filePath });
								}
								for (auto& [id, oldKey] : toRekey) {
									MaterialAsset* asset = AssetManager::GetMaterialAsset(id);
									if (!asset) continue;
									std::string newKey = newPathStr + "::" + newStem;
									asset->name = newStem;
									asset->filePath = newKey;
									AssetManager::RekeyMaterial(oldKey, newKey);
								}
							}
						}
						catch (const std::exception& e) {
							std::cout << "[Assets] Rename failed: " << e.what() << "\n";
						}
					}
					s_RenameTargetPath.clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button(IMGUI_ELEMENT_TITLE("Cancel", "AssetRenameCancel"), ImVec2(80, 0))) {
					s_RenameTargetPath.clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			// ---------- Delete confirmation modal ----------
			if (s_ShowDeleteConfirm) {
				ImGui::OpenPopup("DeleteConfirmPopup");
				s_ShowDeleteConfirm = false;
			}

			if (ImGui::BeginPopupModal("DeleteConfirmPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				std::string itemName = fs::path(s_PendingDeletePath).filename().string();
				if (s_PendingDeleteIsFolder)
					ImGui::Text("Delete folder \"%s\" and ALL its contents?", itemName.c_str());
				else
					ImGui::Text("Delete \"%s\"?", itemName.c_str());
				ImGui::TextDisabled("This cannot be undone.");
				ImGui::Separator();

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
				if (ImGui::Button("Delete", ImVec2(100, 0))) {
					try {
						if (s_PendingDeleteIsFolder) {
							// Unload every .mtl inside the folder before deleting it,
							// so SaveAllMaterials() doesn't recreate the files on next save.
							for (const auto& entry : fs::recursive_directory_iterator(s_PendingDeletePath)) {
								if (!entry.is_regular_file()) continue;
								std::string ext = entry.path().extension().string();
								for (char& c : ext) c = (char)std::tolower((unsigned char)c);
								if (ext == ".mtl")
									AssetManager::UnloadMaterialFile(entry.path().string());
							}
							fs::remove_all(s_PendingDeletePath);
						}
						else {
							// Unload materials before the file is removed.
							std::string ext = fs::path(s_PendingDeletePath).extension().string();
							for (char& c : ext) c = (char)std::tolower((unsigned char)c);
							if (ext == ".mtl")
								AssetManager::UnloadMaterialFile(s_PendingDeletePath);

							fs::remove(s_PendingDeletePath);
						}
						std::cout << "[Assets] Deleted: " << s_PendingDeletePath << "\n";
					}
					catch (const std::exception& e) {
						std::cout << "[Assets] Delete failed: " << e.what() << "\n";
					}
					s_PendingDeletePath.clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::PopStyleColor(2);

				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(100, 0))) {
					s_PendingDeletePath.clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::End();
	}

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
		if (!showConsoleModule)
			return;

		if (ImGui::Begin(IMGUI_ELEMENT_TITLE("Console", "Console"), &showConsoleModule))
		{
			// --- Toolbar: filter + clear ---
			static ImGuiTextFilter filter;
			filter.Draw("##ConsoleFilter", 200.0f);
			ImGui::SameLine();
			if (ImGui::Button(_("Clear")))
				ClearConsole();
			ImGui::SameLine();
			// Level toggles
			static bool showTrace   = true;
			static bool showWarning = true;
			static bool showError   = true;
			ImGui::Checkbox(_("Info"),    &showTrace);
			ImGui::SameLine();
			ImGui::Checkbox(_("Warning"), &showWarning);
			ImGui::SameLine();
			ImGui::Checkbox(_("Error"),   &showError);
			ImGui::Separator();

			// --- Scrolling log area ---
			ImGui::BeginChild("##ConsoleScrollArea", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			if (ImGui::BeginTable("ConsoleOutputTable", 1, ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Output", ImGuiTableColumnFlags_WidthStretch);

				for (const auto& entry : s_ConsoleEntries)
				{
					// Level filter
					if (entry.level == ConsoleEntry::Level::Trace   && !showTrace)   continue;
					if (entry.level == ConsoleEntry::Level::Warning && !showWarning) continue;
					if (entry.level == ConsoleEntry::Level::Error   && !showError)   continue;

					// Text filter
					std::string full = std::format("[{}] {}", entry.source, entry.message);
					if (!filter.PassFilter(full.c_str())) continue;

					ImGui::TableNextRow();
					if (ImGui::TableSetColumnIndex(0))
					{
						if (entry.level == ConsoleEntry::Level::Warning)
							ShowConsoleWarningOutput(entry.source.c_str(), entry.message.c_str());
						else if (entry.level == ConsoleEntry::Level::Error)
							ShowConsoleErrorOutput(entry.source.c_str(), entry.message.c_str());
						else
							ShowConsoleTraceOutput(entry.source.c_str(), entry.message.c_str());
					}
				}

				ImGui::EndTable();
			}

			// Auto-scroll to bottom when new messages arrive
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);

			ImGui::EndChild();
		}
		ImGui::End();
	}

	void ImGuiLayer::ShowControlsModule()
	{
		// control keybinds - hardcoded because not planning on being customizable
		std::vector<std::pair<const char*, const char*>> ControlTextMap =
		{
			{_("Undo"), "CTRL + Z"},
			{_("Redo"), "CTRL + Y"},
			{_("Rotate Viewport Angle"), "RMB"},
			{_("Move Camera Forward"), "RMB + W"},
			{_("Move Camera Backward"), "RMB + S"},
			{_("Move Camera Left"), "RMB + A"},
			{_("Move Camera Right"), "RMB + D"},
			{_("Move Camera Up"), "RMB + E"},
			{_("Move Camera Down"), "RMB + Q"},
			{_("Increase/Decrease Camera Speed"), "Scroll Wheel"},
			//{_("Zoom In"), _("Mouse Scroll Down")},
			//{_("Zoom Out"),_("Mouse Scroll Up")},
			//{_("Create Camera From View"), "CTRL + SHIFT + C"},
			{_("Move"),"1"},
			{_("Rotate"),"2"},
			{_("Scale"),"3"},
			{_("Focus on Selected Object"), "F"},
			{_("Duplicate"),"CTRL + D"},
			{_("Delete"),"DELETE"},
			{_("Play Mode"), "P"}
		};

		if (showControlsModule)
		{
			if (ImGui::Begin(IMGUI_ELEMENT_TITLE("Controls", "Controls"), &showControlsModule))
			{
				// HelpMarker("Control keybinds in Orion are not currently editable!");
				static ImGuiTableFlags flags =
					ImGuiTableFlags_SizingFixedFit |
					ImGuiTableFlags_Hideable;

				if (ImGui::BeginTable("ControlsTable", 2, flags))
				{
					// make control name column fixed width, keybind column stretch
					ImGui::TableSetupColumn(IMGUI_ELEMENT_TITLE("Control", "ControlCol"), ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn(IMGUI_ELEMENT_TITLE("Keybind", "KeybindCol"), ImGuiTableColumnFlags_WidthStretch);
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
			}
			ImGui::End();
		}
	}

	// ========================== PROJECT SETTINGS ==========================

	void ImGuiLayer::ShowProjectSettingsWindow()
	{
		if (!showProjectSettings)
			return;

		ProjectSettings& settings = ProjectSettings::Get();

		ImGui::SetNextWindowSize(ImVec2(480, 520), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(IMGUI_ELEMENT_TITLE("Project Settings", "Project Settings"), &showProjectSettings))
		{
			// ---------- Project Info ----------
			if (ImGui::CollapsingHeader(IMGUI_ELEMENT_TITLE("Project Info", "Project Info"), ImGuiTreeNodeFlags_DefaultOpen))
			{
				char nameBuf[256];
				strncpy_s(nameBuf, settings.projectName.c_str(), sizeof(nameBuf) - 1);
				nameBuf[sizeof(nameBuf) - 1] = '\0';
				if (ImGui::InputText(IMGUI_ELEMENT_TITLE("Project Name", "Project Name"), nameBuf, sizeof(nameBuf)))
					settings.projectName = nameBuf;

				char sceneBuf[512];
				strncpy_s(sceneBuf, settings.startingScene.c_str(), sizeof(sceneBuf) - 1);
				sceneBuf[sizeof(sceneBuf) - 1] = '\0';
				if (ImGui::InputText(IMGUI_ELEMENT_TITLE("Starting Scene", "Starting Scene"), sceneBuf, sizeof(sceneBuf)))
					settings.startingScene = sceneBuf;
			}

			ImGui::Separator();

			// ---------- Background / Sky ----------
			if (ImGui::CollapsingHeader(IMGUI_ELEMENT_TITLE("Background", "Background"), ImGuiTreeNodeFlags_DefaultOpen))
			{
				// Mode selector dropdown
				const char* modeNames[] = { _("Solid Color"), _("Gradient"), _("Cubemap (coming soon)") };
				int currentMode = static_cast<int>(settings.backgroundMode);
				if (ImGui::Combo(IMGUI_ELEMENT_TITLE("Mode", "BackgroundMode"), &currentMode, modeNames, IM_ARRAYSIZE(modeNames)))
				{
					// Don't allow selecting Cubemap yet — snap back to current if attempted
					if (currentMode == static_cast<int>(BackgroundMode::Cubemap))
						currentMode = static_cast<int>(settings.backgroundMode);
					settings.backgroundMode = static_cast<BackgroundMode>(currentMode);
				}

				// Show mode-specific controls
				switch (settings.backgroundMode)
				{
				case BackgroundMode::SolidColor:
				{
					ImGui::ColorEdit3(IMGUI_ELEMENT_TITLE("Color", "BackgroundColor"), &settings.solidColor.x);
					break;
				}
				case BackgroundMode::Gradient:
				{
					ImGui::ColorEdit3(IMGUI_ELEMENT_TITLE("Top Color", "BackgroundTopColor"), &settings.gradientTopColor.x);
					ImGui::ColorEdit3(IMGUI_ELEMENT_TITLE("Bottom Color", "BackgroundBotColor"), &settings.gradientBottomColor.x);
					break;
				}
				case BackgroundMode::Cubemap:
				{
					ImGui::TextDisabled(IMGUI_ELEMENT_TITLE("Cubemap skyboxes are not yet implemented.", "CubemapNotif"));
					ImGui::TextDisabled(IMGUI_ELEMENT_TITLE("This will support 6-face cubemap textures.", "CubemapTextureNotif"));

					char cubeBuf[512];
					strncpy_s(cubeBuf, settings.cubemapPath.c_str(), sizeof(cubeBuf) - 1);
					cubeBuf[sizeof(cubeBuf) - 1] = '\0';

					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					ImGui::InputText(IMGUI_ELEMENT_TITLE("Cubemap Path", "Cubemap Path"), cubeBuf, sizeof(cubeBuf));
					ImGui::PopStyleVar();
					break;
				}
				}
			}

			ImGui::Separator();

			// ---------- Lighting ----------
			if (ImGui::CollapsingHeader(IMGUI_ELEMENT_TITLE("Lighting", "Lighting"), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text(_("Directional Light (Sun)"));
				ImGui::DragFloat3(IMGUI_ELEMENT_TITLE("Direction", "Direction"), &settings.sunDirection.x, 0.01f, -1.0f, 1.0f);
				ImGui::ColorEdit3(IMGUI_ELEMENT_TITLE("Sun Color", "SunColor"), &settings.sunColor.x);
				ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Sun Intensity", "SunIntensity"), &settings.sunIntensity, 0.01f, 0.0f, 10.0f);

				ImGui::Separator();
				ImGui::Text(_("Post-Processing"));
				ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Exposure", "Exposure"), &settings.exposure, 0.01f, 0.1f, 10.0f, "%.2f");
			}

			ImGui::Separator();

			// ---------- Language ----------
			if (ImGui::CollapsingHeader(IMGUI_ELEMENT_TITLE("Language", "ProjSettingLanguage"), ImGuiTreeNodeFlags_DefaultOpen))
			{
				const char* languages[] = { _("English"), _("Spanish") };
				int currentLanguage = static_cast<int>(settings.editorLanguage);

				// language choice drop-down
				if (ImGui::BeginCombo(IMGUI_ELEMENT_TITLE("Choose Editor Language", "LanguageCombo"), languages[currentLanguage]))
				{
					for (int i = 0; i < IM_ARRAYSIZE(languages); i++)
					{
						const bool isSelected = (currentLanguage == i);

						if (ImGui::Selectable(languages[i], isSelected))
						{
							// change editor language when user selects different choice
							currentLanguage = i;
							settings.editorLanguage = static_cast<Language>(currentLanguage);
							Application::Get().SetLocalization(settings.editorLanguage);
						}

						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

			}

			ImGui::Separator();

			// ---------- Debug / Editor ----------
			if (ImGui::CollapsingHeader(IMGUI_ELEMENT_TITLE("Debug Rendering", "Debug Rendering"), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Checkbox(IMGUI_ELEMENT_TITLE("Show World Grid", "Show World Grid"), &settings.showGrid);
				if (settings.showGrid) {
					ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Grid Spacing", "Grid Spacing"), &settings.gridSpacing, 0.25f, 0.25f, 20.0f, "%.2f");
					ImGui::DragFloat(IMGUI_ELEMENT_TITLE("Grid Extent", "Grid Extent"), &settings.gridHalfExtent, 5.0f, 10.0f, 500.0f, "%.0f");
				}

			}

			ImGui::End();
		}

	}

}