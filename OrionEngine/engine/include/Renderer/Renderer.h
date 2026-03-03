#pragma once
#include <cstdint> 
#include <vector>
#include <memory>   // for std::unique_ptr
#include <utility>  // for std::pair
#include <string>

#include "Renderer/IRenderBackend.h"
#include "Renderer/Handles.h"
#include "Renderer/RenderStats.h"
#include "Renderer/RenderScene.h"
#include "Renderer/RenderTarget.h"
#include "Renderer/RenderSettings.h"
#include "Renderer/DebugRenderSettings.h"
#include "Renderer/RenderPass.h"
#include "Scene/Scene.h"
#include "Renderer/Mesh.h"



class Renderer {
public:
    static std::unique_ptr<IRenderBackend> backend;
    // ResourceManager* resources;
    Scene* scene;
    uint64_t frameindex = 0;
    static RenderStats stats;
    RenderScene renderScene;
    RenderTarget viewportTarget;
    std::pair<int, int> viewportSize;
    bool viewportDirty = true;
    CameraHandle activeCamera;
    static RenderSettings settings;
    DebugRenderSettings debugSettings;
    std::vector<std::unique_ptr<RenderPass>> passes;
    RenderTarget pickingTarget;


    static bool Init();
    static void Shutdown();
    static void beginFrame(float dt);
    static void RenderFrame();
    static void EndFrame();
    static void SetScene(Scene* scene);
    static void SyncScene();
    static void SetViewportSize(int w, int h);
    static TextureHandle GetViewportTexture();
    static void RebuildPassList();
    static void ExecutePasses();
    static RenderSettings* GetSettings() { return &settings; }
    static RenderStats* GetStats() { return &stats; }
    static int PickObject();

};