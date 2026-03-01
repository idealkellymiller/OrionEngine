#pragma once


enum class LightType {
    Directional,
    Point,
    Spot
};


class LightData {
public:
    LightType type;
    // Vec3 color;
    // Vec3 position; // For point and spot lights
    // Vec3 direction; // For directional and spot lights
    float intensity;
    float range; // For point and spot lights
};