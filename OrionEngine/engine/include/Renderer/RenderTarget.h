#pragma once
#include <utility>  // for std::pair

#include "Renderer/Handles.h"


class IRenderBackend;   // Forward declaration of IRenderBackend to avoid circular dependency

class RenderTarget{
public:
    std::pair<int, int> size;
    TextureHandle colorTexture;
    TextureHandle depthTexture;
    BufferHandle framebuffer;
    // TextureFormat colorFormat;
    // TextureFormat depthFormat;

    bool IsValid() const;
    void Resize(IRenderBackend* backend, int w, int h);
    void Destroy(IRenderBackend* backend);
    TextureHandle GetColorTexture() const { return colorTexture; }
    TextureHandle GetDepthTexture() const { return depthTexture; }
};
