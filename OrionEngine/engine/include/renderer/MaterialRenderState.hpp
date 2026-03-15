#pragma once


// Which triangle faces should OpenGL cull?
enum class CullMode
{
    None,   // Do not cull any faces
    Back,   // Cull back-facing triangles
    Front   // Cull front-facing triangles
};

// High-level material blend mode.
// We keep this simple for now.
enum class BlendMode
{
    Opaque,      // No blending
    Transparent  // Standard alpha blending
};

// Render state owned by a material.
// This describes HOW the object should be drawn.
struct MaterialRenderState
{
    BlendMode Blend = BlendMode::Opaque;

    bool DepthTest = true;   // Usually enabled for 3D rendering
    bool DepthWrite = true;  // Transparent materials often disable this
    CullMode Cull = CullMode::Back;
};