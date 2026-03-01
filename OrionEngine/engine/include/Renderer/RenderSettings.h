#pragma once


enum class ShadowQuality {
    Low,
    Medium,
    High
};


class RenderSettings {
public:
    bool vsync = true;
    int msaaSamples = 4;
    bool hdrEnabled = false;
    float exposure = 1.0f;
    float gamma = 2.2f;
    bool shadowsEnabled = true;
    ShadowQuality shadowQuality = ShadowQuality::Medium;
    // ClearDesc clear;
};