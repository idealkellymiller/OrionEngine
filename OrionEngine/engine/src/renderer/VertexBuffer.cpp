#include "VertexBuffer.hpp"
#include <glad/glad.h>


VertexBuffer::~VertexBuffer()
{
	Destroy();
}

bool VertexBuffer::Create(const void* data, unsigned int sizeInBytes)
{
	Destroy();

	// Generate one buffer object ID.
	glGenBuffers(1, &m_BufferID);

	// Bind this buffer to the GL_ARRAY_BUFFER target.
	// This target is used for generic vertex data.
	glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);

	// Upload CPU-side data into currently bound buffer object.
	// GL_STATIC_DRAW means we expect the data to be uploaded once
	// and used many times for drawing.
	glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, GL_STATIC_DRAW);

	// Unbind for cleanliness.
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return m_BufferID != 0;
}

void VertexBuffer::Destroy()
{
	if (m_BufferID != 0) {
		// Delete the GPU buffer object
		glDeleteBuffers(1, &m_BufferID);
		m_BufferID = 0;
	}
}

void VertexBuffer::Bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
}

void VertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}