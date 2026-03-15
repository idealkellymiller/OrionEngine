#include "OpenGLBackend.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"
#include "MaterialRenderState.hpp"

#include <glad/glad.h>
#include <iostream>


bool OpenGLBackend::Init() 
{
	// Enables depth testing.
	// This tells OpenGL to compate fragment depth values so that.
	// closer objects appear in front of farther ones.
	glEnable(GL_DEPTH_TEST);

	// Set the depth comparison rule.
	// GL_LESS means a fragment passes if its depth is less than
	// the value already stored in the depth buffer.
	glDepthFunc(GL_LESS);

	// Set the initial clear color.
	// glClearColor does NOT immediately change the screen.
	// It onlt tells OpenGL waht color to use next time the
	// color buffer is cleared with glClear(GL_COLOR_BUFFER_BIT).
	glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);

	return true;
}

void OpenGLBackend::Shutdown()
{
	// Nothing here yet
}

void OpenGLBackend::SetViewport(int x, int y, int width, int height)
{
	// Defines  the drawable area of the framebuffer.
	// (x,y) is usually the bottom-left corner in OpenGL.
	// width and height are the size of the render region in pixels.
	//
	// This is important when the window changes size.
	// If the viewport is wrong, rendering can appear stretched,
	// clipped, or only use part of the window.
	glViewport(x, y, width, height);
}

void OpenGLBackend::SetClearColor(float r, float g, float b, float a)
{
	// Update the stored clear color.
	m_ClearColor[0] = r;
	m_ClearColor[1] = g;
	m_ClearColor[2] = b;
	m_ClearColor[3] = a;

	// Stores the RGBA color OpenGL should use during a color-buffer clear.
	// Again, this does not paint the window by itself.
	glClearColor(r, g, b, a);
}

void OpenGLBackend::BeginFrame()
{
	glEnable(GL_DEPTH_TEST);

	// Clear the color buffer using the color previously set by glClearColor.
	// The color buffer is what ends up being displayed on screen.
	//
	// Clears the depth buffer as well.
	// The depth buffer stores per-pixel depth information used for
	// proper 3D visibilty testing.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLBackend::EndFrame()
{
	// In this simple design, nothing happens here yet.
	//
	// Why?
	// Because GLFW handles buffer swapping with glfwSwapBuffers(window),
	// which is not an OpenGL function. It belongs to the window system layer.
	//
	// Later, EndFrame may be used for statistics, command flushing,
	// debug markers, or post-processing flow.
}

void OpenGLBackend::Draw(const VertexArray& vertexArray, int vertexCount)
{
	// Bind the VAO, which contains the vertex layout configuration.
	vertexArray.Bind();

	// non-indexed draw: read vertices sequentially from the bound vertex buffer.
	glDrawArrays(GL_TRIANGLES, 0, vertexCount);

	// Optional cleanup
	vertexArray.Unbind();
}

void OpenGLBackend::DrawIndexed(const VertexArray& vertexArray, int indexCount)
{
	vertexArray.Bind();

	// Indexed draw: read indices fro the element array buffer currently associated with the VAO.
	//
	// Parameters:
	// - GL_TRIANGLES: topology
	// - indexCount: how many indices to consume
	// - GL_UNSIGNED_INT: type of each index in the index buffer
	// - nullptr: start at offset 0 in the bound index buffer
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

	vertexArray.Unbind();
}

void OpenGLBackend::ApplyMaterialRenderState(const Material& material)
{
	const MaterialRenderState& state = material.GetRenderState();


	// Depth testing controls whether fragments are compared against the depth buffer.
	if (state.DepthTest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	// Depth write controls whether the fragment updates the depth buffer.
	glDepthMask(state.DepthWrite ? GL_TRUE : GL_FALSE);

	// Configure face culling
	switch (state.Cull) {
	case CullMode::None:
		glDisable(GL_CULL_FACE);
		break;

	case CullMode::Back:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);	// Cull back-facing triangles
		break;

	case CullMode::Front:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);	// Cull front-facing trianlges
		break;
	}

	// Configure blending.
	if (state.Blend == BlendMode::Transparent) {
		glEnable(GL_BLEND);

		// Standard alpha blending:
		// finalColor = srcColor * srcAlpha + dstColor * (1-srcAlpha)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else {
		glDisable(GL_BLEND);
	}
}