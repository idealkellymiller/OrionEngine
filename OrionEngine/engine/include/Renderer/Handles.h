#pragma once


struct MeshHandle { uint32_t id = 0; };      // 0 = invalid
struct TextureHandle { uint32_t id = 0; };
struct MaterialHandle { uint32_t id = 0; };
struct PipelineHandle { uint32_t id = 0; };
struct ShaderHandle { uint32_t id = 0; };
struct BufferHandle { uint32_t id = 0; };
struct CameraHandle { uint32_t id = 0; };

inline bool IsValid(MeshHandle h) { return h.id != 0; }


struct MeshDesc {};
struct TextureDesc {};
struct MaterialDesc {};
struct PipelineDesc {};
struct ShaderDesc {};
struct BufferDesc {};
struct CameraDesc {};
struct RenderTargetDesc {};
