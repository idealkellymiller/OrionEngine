#pragma once
#include <cstdint>
#include <utility>

#include "Renderer/RenderScene.h"
#include "Renderer/RenderTarget.h"
#include "Renderer/CameraData.h"
#include "Renderer/RenderSettings.h"
#include "Renderer/DebugRenderSettings.h"
#include "RenderStats.h"


class FrameContext {
public:
    uint32_t frameIndex;
    float deltaTime;
    RenderScene* renderScene;
    RenderTarget* viewportTarget;
    CameraData* camera;
    RenderSettings* settings;
    DebugRenderSettings* debugSettings;
    RenderStats* stats;
    RenderTarget* pickingTarget;
    std::pair<int, int> viewportSize;
};