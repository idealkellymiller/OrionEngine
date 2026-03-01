#pragma once
#include "Renderer/IRenderBackend.h"


class OpenGLBackend : public IRenderBackend {
public:
    bool Init() override;
    void Shutdown() override;
    void BeginFrame(float dt) override;
    void EndFrame() override;
    void Present() override; 
    void SetViewport(int x, int y, int w, int h) override;
    void SetDepthTest(bool enabled) override;
    void SetCullMode(CullMode mode) override;
    void SetBlendMode(BlendMode mode) override;
    void SetWireFrame(bool enabled) override;
    void BindRenderTarget(const RenderTarget& target) override;
    void Clear() override;
    void BindPipeline(PipelineHandle pipeline) override;
    void BindShader(ShaderHandle shader) override;
    void BindMaterial(MaterialHandle material) override;
    void BindMesh(MeshHandle mesh) override;
    void DrawIndexed(uint32_t indexCount) override;
    void DrawInstance(uint32_t instanceCount) override;
    BufferHandle CreateBuffer(const BufferDesc& desc, const void* data = nullptr) override;
    TextureHandle CreateTexture(const TextureDesc& desc, const void* data = nullptr) override;
    ShaderHandle CreateShader(const ShaderDesc& desc) override;
    PipelineHandle CreatePipeline(const PipelineDesc& desc) override;
    RenderTarget CreateRenderTarget(const RenderTargetDesc& desc) override;
    void DestroyBuffer(BufferHandle buffer) override;
    void DestroyTexture(TextureHandle texture) override;
    void DestroyShader(ShaderHandle shader) override;
    void DestroyPipeline(PipelineHandle pipeline) override;
    void DestroyRenderTarget(RenderTarget target) override;
    void DestroyMesh(MeshHandle mesh) override;
    void DestroyMaterial(MaterialHandle material) override;
};