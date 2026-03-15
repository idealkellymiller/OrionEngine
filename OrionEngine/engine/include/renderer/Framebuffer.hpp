#pragma once
#include <glad/glad.h>


// Simple OpenGL framebuffer wrapper
// This will own
// - an FBO
// - a color texture attachment
// - a depth/stencil renderbuffer
class Framebuffer {
public:
	Framebuffer();
	~Framebuffer();

	// Create the framebuffer and attachments
	void Create(unsigned int width, unsigned int height);

	// Free all OpenGL objects
	void Destroy();

	// Rebuild attachments at a new size
	void Resize(unsigned int width, unsigned int height);
	
	// Bind this framebuffer so future rendering goes into it
	void Bind() const;

	// Bind the default framebuffer (the GLFW window backbuffer)
	void Unbind() const;

	// Get the color texture so ImGui can display it
	unsigned int GetColorAttachment() const { return m_ColorAttachment; }

	unsigned int GetWidth() const { return m_Width; }
	unsigned int GetHeight() const { return m_Height; }

private:
	unsigned int m_FBO;
	unsigned int m_ColorAttachment;
	unsigned int m_DepthStencilRBO;

	unsigned int m_Width;
	unsigned int m_Height;
};