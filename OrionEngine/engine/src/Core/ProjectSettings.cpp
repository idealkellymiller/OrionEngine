#include "Core/ProjectSettings.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace Orion {

    // ---------- Helper: glm ↔ JSON ----------

    static json Vec3ToJson(const glm::vec3& v)
    {
        return { v.x, v.y, v.z };
    }

    static json Vec4ToJson(const glm::vec4& v)
    {
        return { v.x, v.y, v.z, v.w };
    }

    static glm::vec3 JsonToVec3(const json& j, const glm::vec3& fallback)
    {
        if (!j.is_array() || j.size() < 3) return fallback;
        return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>() };
    }

    static glm::vec4 JsonToVec4(const json& j, const glm::vec4& fallback)
    {
        if (!j.is_array() || j.size() < 4) return fallback;
        return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>() };
    }

    static Language LocaleToLanguage(std::string_view locale)
    {
        if (locale == "en") { return Language::English; }
        else if (locale == "es-ES") { return Language::Spanish; }

        // default langauge
        return Language::English;

    }

    const std::string_view ProjectSettings::LanguageToLocale(Language lang)
    {
        switch (lang)
        {
        case Language::English:
            return "en";
        case Language::Spanish:
            return "es-ES";
        default:
            return "en";
        }
    }

    // ---------- Save ----------

    bool ProjectSettings::Save(const std::string& filePath) const
    {
        json root;

        // Project info
        root["project"]["name"]          = projectName;
        root["project"]["startingScene"] = startingScene;

        // Background
        root["background"]["mode"]               = static_cast<int>(backgroundMode);
        root["background"]["solidColor"]         = Vec4ToJson(solidColor);
        root["background"]["gradientTopColor"]   = Vec4ToJson(gradientTopColor);
        root["background"]["gradientBottomColor"]= Vec4ToJson(gradientBottomColor);
        root["background"]["cubemapPath"]        = cubemapPath;

        // Lighting
        root["lighting"]["sunDirection"]     = Vec3ToJson(sunDirection);
        root["lighting"]["sunColor"]         = Vec3ToJson(sunColor);
        root["lighting"]["sunIntensity"]     = sunIntensity;
        root["lighting"]["ambientIntensity"] = ambientIntensity;

        // Language
        root["language"]["locale"] = LanguageToLocale(editorLanguage);

        // Debug / Editor
        root["debug"]["showGrid"]       = showGrid;
        root["debug"]["gridSpacing"]    = gridSpacing;
        root["debug"]["gridHalfExtent"] = gridHalfExtent;

        // Write to disk
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cout << "[ProjectSettings] Failed to save to: " << filePath << "\n";
            return false;
        }

        file << root.dump(4);  // pretty-print with 4-space indent
        file.close();

        std::cout << "[ProjectSettings] Saved to: " << filePath << "\n";
        return true;
    }

    // ---------- Load ----------

    bool ProjectSettings::Load(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            // No settings file yet — keep defaults. Not an error.
            std::cout << "[ProjectSettings] No settings file found at: " << filePath
                      << " — using defaults.\n";
            return false;
        }

        json root;
        try {
            file >> root;
        }
        catch (const json::parse_error& e) {
            std::cout << "[ProjectSettings] Parse error: " << e.what() << "\n";
            return false;
        }
        file.close();

        // Defaults used as fallbacks for any missing key.
        ProjectSettings defaults;

        // Project info
        if (root.contains("project")) {
            auto& p = root["project"];
            if (p.contains("name"))          projectName   = p["name"].get<std::string>();
            if (p.contains("startingScene")) startingScene = p["startingScene"].get<std::string>();
        }

        // Background
        if (root.contains("background")) {
            auto& bg = root["background"];
            if (bg.contains("mode"))
                backgroundMode = static_cast<BackgroundMode>(bg["mode"].get<int>());
            if (bg.contains("solidColor"))
                solidColor = JsonToVec4(bg["solidColor"], defaults.solidColor);
            if (bg.contains("gradientTopColor"))
                gradientTopColor = JsonToVec4(bg["gradientTopColor"], defaults.gradientTopColor);
            if (bg.contains("gradientBottomColor"))
                gradientBottomColor = JsonToVec4(bg["gradientBottomColor"], defaults.gradientBottomColor);
            if (bg.contains("cubemapPath"))
                cubemapPath = bg["cubemapPath"].get<std::string>();
        }

        // Lighting
        if (root.contains("lighting")) {
            auto& lt = root["lighting"];
            if (lt.contains("sunDirection"))
                sunDirection = JsonToVec3(lt["sunDirection"], defaults.sunDirection);
            if (lt.contains("sunColor"))
                sunColor = JsonToVec3(lt["sunColor"], defaults.sunColor);
            if (lt.contains("sunIntensity"))
                sunIntensity = lt["sunIntensity"].get<float>();
            if (lt.contains("ambientIntensity"))
                ambientIntensity = lt["ambientIntensity"].get<float>();
        }

        // Language
        if (root.contains("language")) {
            auto& lg = root["language"];
            if (lg.contains("locale"))
                editorLanguage = LocaleToLanguage(lg["locale"]);
        }

        // Debug / Editor
        if (root.contains("debug")) {
            auto& db = root["debug"];
            if (db.contains("showGrid"))       showGrid       = db["showGrid"].get<bool>();
            if (db.contains("gridSpacing"))    gridSpacing    = db["gridSpacing"].get<float>();
            if (db.contains("gridHalfExtent")) gridHalfExtent = db["gridHalfExtent"].get<float>();
        }

        std::cout << "[ProjectSettings] Loaded from: " << filePath << "\n";
        return true;
    }

}
