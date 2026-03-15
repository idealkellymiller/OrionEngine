#include "Framebuffer.hpp"
#include <iostream>


Framebuffer::Framebuffer() : m_FBO(0), m_ColorAttachment(0), m_DepthStencilRBO(0), m_Width(0), m_Height(0)
{
}

Framebuffer::~Framebuffer()
{
    Destroy();
}

void Framebuffer::Create(unsigned int width, unsigned int height)
{
    m_Width = width;
    m_Height = height;

    // Generate and bind the framebuffer object
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // Create the color texture that the scene will render into
    glGenTextures(1, &m_ColorAttachment);
    glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);

    // Allocate empty texture storage
    // GL_RGB is enough for a simple viewport color buffer
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr);

    // Set texture filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Optional wrap mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach the texture to the framebuffer as a color attachment 0
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        m_ColorAttachment,
        0);

    // Create a renderbuffer for depth/stencil testing
    glGenRenderbuffers(1, &m_DepthStencilRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRBO);

    // Allocate depth/stencil storage
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height)
    );

    // Attach renderbuffer to framebuffer
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        m_DepthStencilRBO
    );

    // Check that the framebuffer is complete and valid
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is incomplete.\n";
    }

    // Unbind framebuffer so we do not accidentally render into it.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Destroy()
{
    if (m_DepthStencilRBO != 0)
    {
        glDeleteRenderbuffers(1, &m_DepthStencilRBO);
        m_DepthStencilRBO = 0;
    }

    if (m_ColorAttachment != 0)
    {
        glDeleteTextures(1, &m_ColorAttachment);
        m_ColorAttachment = 0;
    }

    if (m_FBO != 0)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }

    m_Width = 0;
    m_Height = 0;
}

void Framebuffer::Resize(unsigned int width, unsigned int height)
{
    // Avoid invalid framebuffer sizes
    if (width == 0 || height == 0)
        return;

    // Skip unnecessary rebuild
    if (m_Width == width && m_Height == height)
        return;

    Destroy();
    Create(width, height);
}

void Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
}

void Framebuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}