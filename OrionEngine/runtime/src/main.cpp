// OrionEngine Standalone Game Runtime
// Entry point for exported/built games.
// No editor UI, no ImGui — just the game running.

#include "Orion.h"
#include "Core/GameSettings.h"
#include "Layers/RuntimeLayer.h"
#include "Renderer/Renderer.h"
#include "Assets/AssetManager.h"
#include "ECS/SceneManager.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// Minimal render layer: calls Renderer::Render() and blits to screen.
// Replaces EditorLayer (which handles gizmos, picking, editor camera).
class GameRenderLayer : public Orion::Layer {
public:
    GameRenderLayer() : Layer("GameRenderLayer") {}

    void OnUpdate() override {
        Orion::Renderer::Render();
        Orion::Renderer::BlitToScreen();
    }

    void OnEvent(Orion::Event& event) override {}
};

class GameApplication : public Orion::Application {
public:
    GameApplication(const Orion::WindowProperties& props)
        : Application(props)
    {
        PushLayer(new GameRenderLayer());

        m_RuntimeLayer = new Orion::RuntimeLayer();
        PushLayer(m_RuntimeLayer);
    }

    ~GameApplication() {}

    Orion::RuntimeLayer* GetRuntimeLayer() { return m_RuntimeLayer; }

private:
    Orion::RuntimeLayer* m_RuntimeLayer = nullptr;
};

int main(int argc, char** argv)
{
    Orion::Log::Init();

    // Determine where the exe lives
    fs::path exeDir = fs::absolute(fs::path(argv[0]).parent_path());
    if (exeDir.empty()) exeDir = fs::current_path();

    // Set working directory to the exe's folder (bin/).
    // The engine loads shaders from "../engine/shaders/" relative to cwd,
    // so cwd must be one level below the engine/ folder.
    fs::current_path(exeDir);

    // Load game settings
    Orion::GameSettings settings;
    settings.Load((exeDir / "game.settings").string());

    // Resolve assets folder: engine expects "../editor/assets/" from bin/
    std::string assetsPath = (exeDir / ".." / "editor" / "assets").string() + "\\";
    if (!fs::exists(assetsPath)) {
        // Fallback: assets/ next to the exe
        assetsPath = (exeDir / "assets").string() + "\\";
    }

    std::string scenePath = assetsPath + settings.startScene;

    if (!fs::exists(scenePath)) {
        std::cerr << "[Runtime] ERROR: Scene not found: " << scenePath << "\n";
        return 1;
    }

    std::cout << "[Runtime] " << settings.title << "\n";
    std::cout << "[Runtime] Scene: " << scenePath << "\n";

    // Create window and application
    Orion::WindowProperties winProps(
        settings.title,
        settings.width,
        settings.height,
        settings.fullscreen
    );

    GameApplication app(winProps);

    // Initialize the renderer (loads shaders)
    Orion::Renderer::Init();

    // Load assets, load scene, init scripts + physics, start playing
    app.GetRuntimeLayer()->BeginPlayStandalone(scenePath, assetsPath);

    // Run the main loop until the window is closed
    app.RunLoop();

    return 0;
}
