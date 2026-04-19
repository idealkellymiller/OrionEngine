#pragma once
#include "Application.h"
#include "Log/Log.h"
#include "Renderer/Renderer.h"
#include "Assets/AssetManager.h"
#include "ECS/SceneManager.h"
#include "Core/Input.h"

#include <filesystem>

#if ORN_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace Orion {

namespace fs = std::filesystem;

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;
	EditorLayer* Application::s_EditorLayer = nullptr;
	float        Application::s_TimeScale = 1.0f;

	Application::Application(const WindowProperties& props)
	{

		s_Instance = this;
		// sets this application's Window reference to Window instance
		m_Window = std::unique_ptr<Window>(Window::Create(props));
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
		layer->OnAttach();
	}

	void Application::PopLayer(Layer* layer)
	{
		m_LayerStack.PopLayer(layer);
	}

	void Application::PopOverlay(Layer* layer)
	{
		m_LayerStack.PopOverlay(layer);
	}

	void Application::QueueLayerOp(std::function<void()> op)
	{
		m_PendingLayerOps.push_back(std::move(op));
	}

	void Application::ProcessPendingLayerOps()
	{
		for (auto& op : m_PendingLayerOps)
			op();
		m_PendingLayerOps.clear();
	}

	// after receiving an event, dispatch to the layers of the app
	void Application::OnEvent(Event& e)
	{
		// Feed input-relevant events (scroll) into the Input cache before layers
		// see the event, so Input.GetScrollDelta() is accurate whenever queried.
		Input::OnEvent(e);

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

	void Application::SetLocalization(Language lang)
	{
		std::string_view locale = ProjectSettings::LanguageToLocale(lang);
		std::cout << "Setting locale to " << locale << std::endl;
#if ORN_PLATFORM_WINDOWS
		// convert std::string_view locale to lpcwstr for LocaleNameToLCID
		const auto wStringSize = MultiByteToWideChar(CP_UTF8, 0, locale.data(), static_cast<int>(locale.length()), nullptr, 0);
		std::wstring localeName;
		localeName.reserve(wStringSize);
		MultiByteToWideChar(CP_UTF8, 0, locale.data(), static_cast<int>(locale.length()), localeName.data(), wStringSize);

		// configure all threads to the project settings locale
		_configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
		const auto localeID = LocaleNameToLCID(localeName.c_str(), LOCALE_ALLOW_NEUTRAL_NAMES);
		SetThreadLocale(localeID);
#else
		setlocale(LC_MESSAGES, locale.data());
#endif

		bindtextdomain(GETTEXT_DOMAIN, GETTEXT_OUTPUT_DIR);
		bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");
		textdomain(GETTEXT_DOMAIN);
	}

	void Application::Run(int argc, char** argv)
	{
		// Set the working directory to <engine-root>/editor/ so every relative path
		// in the codebase resolves correctly in both dev and installed builds:
		//
		//   ../engine/shaders/   -> <root>/engine/shaders/   (Renderer shaders)
		//   ../editor/assets/    -> <root>/editor/assets/    (scene / asset files)
		//
		// We locate <engine-root> by walking up from the exe until we find a directory
		// that contains engine/shaders/.  This works regardless of how many levels deep
		// the exe lives (dev: build/bin/Debug--Windows/; installed: bin/).
#if ORN_PLATFORM_WINDOWS
		{
			wchar_t exePathW[MAX_PATH] = {};
			GetModuleFileNameW(nullptr, exePathW, MAX_PATH);
			fs::path dir = fs::path(exePathW).parent_path();

			fs::path engineRoot;
			for (int i = 0; i < 6; ++i) {
				if (fs::exists(dir / "engine" / "shaders")) {
					engineRoot = dir;
					break;
				}
				if (dir.has_parent_path() && dir != dir.parent_path())
					dir = dir.parent_path();
				else
					break;
			}

			if (!engineRoot.empty()) {
				fs::path workDir = engineRoot / "editor";
				fs::create_directories(workDir);   // ensure editor/ exists on fresh installs
				SetCurrentDirectoryW(workDir.c_str());
				std::cout << "[App] Working directory set to: " << workDir.string() << "\n";
			} else {
				std::cout << "[App] Warning: could not locate engine root; shaders may not load.\n";
			}
		}
#endif

		// initialize Renderer here, after window creation in main.
		Orion::Renderer::Init();

		// Determine assets folder and startup scene.
		// Priority:
		//   1. A .scene file passed on the command line (e.g. double-click from Explorer)
		//   2. assets\ right next to the exe (installed / flat layout)
		//   3. ..\editor\assets\ (dev / build-game layout where exe lives in bin\)
		std::string assetsFolderPath;
		std::string startupScene = "default.scene";

		if (argc > 1) {
			fs::path scenePath = fs::absolute(argv[1]);
			if (fs::exists(scenePath) && scenePath.extension() == ".scene") {
				assetsFolderPath = scenePath.parent_path().string() + "\\";
				startupScene = scenePath.filename().string();
				std::cout << "[App] Opened scene from command line: " << scenePath.string() << "\n";
			}
		}

		if (assetsFolderPath.empty()) {
			// Working directory is always <engine-root>/editor/ (set above),
			// so assets live at assets/ relative to that.
			assetsFolderPath = "assets\\";
			std::cout << "[App] Assets folder: " << assetsFolderPath << "\n";
		}

		// Load Assets — user assets first, then built-in engine assets
		AssetManager::SetAssetsFolderPath(assetsFolderPath);
		AssetManager::LoadAssetsFolder();

		// Engine assets live at <engine-root>/engine/engineAssets/ relative to the
		// working directory which is always <engine-root>/editor/.
		{
			fs::path engineAssetsPath = fs::absolute("../engine/engineAssets/");
			if (fs::exists(engineAssetsPath)) {
				std::string engPath = engineAssetsPath.string();
				// Ensure trailing separator
				if (!engPath.empty() && engPath.back() != '\\' && engPath.back() != '/')
					engPath += '\\';
				AssetManager::SetEngineAssetsFolderPath(engPath);
				AssetManager::LoadEngineAssetsFolder();
			} else {
				std::cout << "[App] Engine assets not found at: " << engineAssetsPath.string() << "\n";
			}
		}

		// Load project settings (from the assets folder, next to scene files).
		// If the file doesn't exist yet, defaults are kept.
		ProjectSettings::Get().Load(AssetManager::GetAssetsFolderPath() + "project.settings");

		// set localization
		SetLocalization(ProjectSettings::Get().editorLanguage);

		// Load (startup) Scene — only if the file actually exists.
		// A fresh install with no Sample Project has no scene file; just start empty.
		{
			std::string scenePath = AssetManager::GetAssetsFolderPath() + startupScene;
			if (fs::exists(scenePath))
				SceneManager::LoadScene(scenePath);
			else
				std::cout << "[App] No startup scene found at '" << scenePath << "' — starting with empty scene.\n";
		}

		//--------------------------MAIN APP LOOP--------------------------
		while (m_Running) {

			// Process any deferred layer push/pop operations safely
			// before iterating the layer stack.
			ProcessPendingLayerOps();

			// Snapshot keyboard/mouse state and publish per-frame scroll delta
			// so all layers this frame see consistent Input values.
			Input::NewFrame();

			// update every layer in the stack in order
			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate();
			}

			// update app window
			m_Window->OnUpdate();
		}

		// Save project settings on exit so they persist between runs.
		ProjectSettings::Get().Save(AssetManager::GetAssetsFolderPath() + "project.settings");
	}

	void Application::RunLoop()
	{
		while (m_Running) {
			ProcessPendingLayerOps();

			Input::NewFrame();

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate();
			}

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
}
