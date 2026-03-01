#pragma once
#include <vector>

#include "Renderer/Renderable.h"
#include "Renderer/LightData.h"
#include "Renderer/Handles.h"
#include "Renderer/CameraData.h"


class RenderScene{
public:
    std::vector<Renderable> renderables;
    std::vector<LightData> lights;
    CameraHandle activeCamera;

    void Clear();
    void AddRenderable(const Renderable& renderable);
    void AddLight(const LightData& light);
    std::vector<Renderable>& GetRenderables() { return renderables; }
    std::vector<LightData>& GetLights() { return lights; }
    CameraData GetActiveCamera() const;
};