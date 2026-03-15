#pragma once

#include "IRenderBackend.hpp"
#include "Material.hpp"


class VertexArray;

class OpenGLBackend : public IRenderBackend {
public:
	bool Init() override;
	void Shutdown() override;

	void SetViewport(int x, int y, int width, int height) override;
	void SetClearColor(float r, float g, float b, float a) override;

	void BeginFrame() override;
	void EndFrame() override;

	void Draw(const VertexArray& vertexArray,  int vertexCount) override;
	void DrawIndexed(const VertexArray& vertexArray, int indexCount) override;

	void ApplyMaterialRenderState(const Material& material) override;

private:
	// Store the clear color so the backend has a consistent state.
	float m_ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

};