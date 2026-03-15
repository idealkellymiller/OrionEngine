#include "IndexBuffer.hpp"
#include <glad/glad.h>
#include <iostream>



IndexBuffer::~IndexBuffer()
{
	Destroy();
}

bool IndexBuffer::Create(const unsigned int* data, unsigned int count)
{
	Destroy();


	if (data == nullptr || count == 0)
		return false;

	m_Count = count;

	// Generate on buffer object for element/index data.
	glGenBuffers(1, &m_BufferID);

	// Bind this buffer to the element array buffer target.
	// This target is specifically for index data ised by glDrawElements.

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferID);

	// Upload the index data to GPU memory
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		count * sizeof(unsigned int),
		data,
		GL_STATIC_DRAW);

	// Unbind for cleanliness
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return m_BufferID != 0;
}

void IndexBuffer::Destroy()
{
	if (m_BufferID != 0) {
		glDeleteBuffers(1, &m_BufferID);
		m_BufferID = 0;
	}

	m_Count = 0;
}

void IndexBuffer::Bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferID);
}

void IndexBuffer::Unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// Important OpenGL note: 
// EBO binding is part of the VAO state