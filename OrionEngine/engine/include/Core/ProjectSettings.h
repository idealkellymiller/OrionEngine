#pragma once
#include "EngineCore.h"
#include <glm/glm.hpp>
#include <string>

namespace Orion {

    // How the viewport background is rendered each frame.
    enum class BackgroundMode {
        SolidColor,   // Single flat color
        Gradient,     // Vertical blend from top color to bottom color
        Cubemap       // (Future) 6-face cubemap skybox
    };

    // language of all editor text - for i18n
    enum class Language {
        English,
        Spanish
    };

    // Central, engine-wide project settings.
    // Accessed anywhere via ProjectSettings::Get().
    struct ORION_API ProjectSettings {

        // ---------- Background / Sky ----------
        BackgroundMode backgroundMode = BackgroundMode::SolidColor;

        // Solid-color mode
        glm::vec4 solidColor = { 0.4f, 0.4f, 0.4f, 1.0f };

        // Gradient mode
        glm::vec4 gradientTopColor    = { 0.15f, 0.15f, 0.35f, 1.0f };  // dark blue
        glm::vec4 gradientBottomColor = { 0.55f, 0.55f, 0.55f, 1.0f };  // light gray

        // Cubemap mode (future — path to a folder with 6 face textures)
        std::string cubemapPath;

        // ---------- Lighting ----------
        glm::vec3 sunDirection  = { -0.5f, -1.0f, -0.2f };
        glm::vec3 sunColor      = {  1.0f, 0.95f, 0.85f };
        float     sunIntensity  = 1.2f;

        float     ambientIntensity = 0.15f;

        // ---------- Language ----------
        Language editorLanguage = Language::English;

        // ---------- Debug / Editor ----------
        bool showGrid = true;                  // XZ world grid in the viewport
        float gridSpacing = 1.0f;              // distance between grid lines
        float gridHalfExtent = 50.0f;          // how far the grid extends from origin

        // ---------- Project Info ----------
        std::string projectName   = "OrionEngine Project";
        std::string startingScene = "default.scene";

        // ---------- Persistence ----------
        // Save all settings to a JSON file.
        bool Save(const std::string& filePath) const;

        // Load settings from a JSON file. Missing keys keep their defaults.
        bool Load(const std::string& filePath);

        // ---------- Singleton access ----------
        static ProjectSettings& Get() {
            static ProjectSettings s_Instance;
            return s_Instance;
        }

    private:
        ProjectSettings() = default;
    };

}
