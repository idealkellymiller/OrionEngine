/// <summary>
/// These structs are data containers used in initializing new GPU objects.
/// They are backend-agnostic and describe 'what' we want, not 'how' it is implemented.
/// <summary>
#pragma once
#include <cstdint>
#include <vector>


// Render Pass
struct RenderPassDesc {
	float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
	float clearDepth = 1.0f;
	bool  clearColorEnabled = true;
	bool  clearDepthEnabled = true;
};

// Texture
struct TextureDesc {
	uint32_t width = 0;
	uint32_t height = 0;
};

// Render Target
struct RenderTargetDesc {
	uint32_t width = 0;
	uint32_t height = 0;
};