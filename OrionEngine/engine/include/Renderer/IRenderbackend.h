/// <summary>
/// IRenderBackend is an interface that defines the core rendering operations that any rendering backend (like OpenGL, DirectX, Vulkan) must implement.
/// This allows the Renderer class to be decoupled from the specific graphics API, enabling flexibility and easier maintenance.
/// Each method in this interface corresponds to a fundamental rendering operation, such as initializing the backend, setting render states, binding resources, and issuing draw calls.
/// </summary>

#pragma once
#include <cstdint>

#include "Renderer/RenderTarget.h"
#include "Renderer/Handles.h"


enum class CullMode {
    None,
    Front,
    Back,
    FrontAndBack
};

enum class BlendMode {
    None,
    Alpha,
    Additive,
    Multiply
};



class IRenderBackend {
public:
    // Using '= 0' makes this a pure virtual function, meaning that any class that inherits from IRenderBackend must implement this function. 
    // It also makes IRenderBackend an abstract class, so you cannot create instances of it directly.
    virtual bool Init() = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame(float dt) = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0; 
    virtual void SetViewport(int x, int y, int w, int h) = 0;
    virtual void SetDepthTest(bool enabled) = 0;
    virtual void SetCullMode(CullMode mode) = 0;
    virtual void SetBlendMode(BlendMode mode) = 0;
    virtual void SetWireFrame(bool enabled) = 0;
    virtual void BindRenderTarget(const RenderTarget& target) = 0;
    virtual void Clear() = 0;
    virtual void BindPipeline(PipelineHandle pipeline) = 0;
    virtual void BindShader(ShaderHandle shader) = 0;
    virtual void BindMaterial(MaterialHandle material) = 0;
    virtual void BindMesh(MeshHandle mesh) = 0;
    virtual void DrawIndexed(uint32_t indexCount) = 0;
    virtual void DrawInstance(uint32_t instanceCount) = 0;
    virtual BufferHandle CreateBuffer(const BufferDesc& desc, const void* data = nullptr) = 0;
    virtual TextureHandle CreateTexture(const TextureDesc& desc, const void* data = nullptr) = 0;
    virtual ShaderHandle CreateShader(const ShaderDesc& desc) = 0;
    virtual PipelineHandle CreatePipeline(const PipelineDesc& desc) = 0;
    virtual RenderTarget CreateRenderTarget(const RenderTargetDesc& desc) = 0;
    virtual void DestroyBuffer(BufferHandle buffer) = 0;
    virtual void DestroyTexture(TextureHandle texture) = 0;
    virtual void DestroyShader(ShaderHandle shader) = 0;
    virtual void DestroyPipeline(PipelineHandle pipeline) = 0;
    virtual void DestroyRenderTarget(RenderTarget target) = 0;
    virtual void DestroyMesh(MeshHandle mesh) = 0;
    virtual void DestroyMaterial(MaterialHandle material) = 0;
};