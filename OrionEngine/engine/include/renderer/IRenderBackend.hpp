#pragma once


class Material;
class Shader;
class VertexArray;

class IRenderBackend {
public:
	virtual ~IRenderBackend() = default;

	// Called once after the graphics API is ready
	virtual bool Init() = 0;
	// Called when shutting the renderer down
	virtual void Shutdown() = 0;

	// Sets the portion of the window we are rendering into
	virtual void SetViewport(int x, int y, int width, int height) = 0;
	// Sets the color OpenGL will use when clearing the color buffer
	virtual void SetClearColor(float r, float g, float b, float a) = 0;

	// Begins a frame. For noe this will mostly clear buffers
	virtual void BeginFrame() = 0;
	// Ends a frame. In OpenGL this usually does not do much by itself
	// Buffer swapping is usually handled by the window system (GLFW)
	virtual void EndFrame() = 0;

	virtual void Draw(const VertexArray& vertexArray, int vertexCount) = 0;

	// new indexed path
	virtual void DrawIndexed(const VertexArray& vertexArray, int indexCount) = 0;

	virtual void ApplyMaterialRenderState(const Material& material) = 0;
};