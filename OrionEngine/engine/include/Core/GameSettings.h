#pragma once
#include "EngineCore.h"
#include <string>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace Orion {

    struct GameSettings {
        std::string title = "OrionEngine Game";
        unsigned int width = 1920;
        unsigned int height = 1080;
        bool fullscreen = false;
        std::string startScene = "default.scene";

        bool Load(const std::string& path) {
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cout << "[GameSettings] Could not open: " << path << "\n";
                return false;
            }

            try {
                nlohmann::json j;
                file >> j;

                if (j.contains("title"))      title      = j["title"].get<std::string>();
                if (j.contains("width"))       width      = j["width"].get<unsigned int>();
                if (j.contains("height"))      height     = j["height"].get<unsigned int>();
                if (j.contains("fullscreen"))  fullscreen = j["fullscreen"].get<bool>();
                if (j.contains("startScene"))  startScene = j["startScene"].get<std::string>();

                std::cout << "[GameSettings] Loaded: " << path << "\n";
                return true;
            }
            catch (const std::exception& e) {
                std::cout << "[GameSettings] Parse error: " << e.what() << "\n";
                return false;
            }
        }

        void Save(const std::string& path) const {
            nlohmann::json j;
            j["title"]      = title;
            j["width"]      = width;
            j["height"]     = height;
            j["fullscreen"] = fullscreen;
            j["startScene"] = startScene;

            std::ofstream file(path);
            if (file.is_open()) {
                file << j.dump(4);
                std::cout << "[GameSettings] Saved: " << path << "\n";
            }
        }
    };

}
