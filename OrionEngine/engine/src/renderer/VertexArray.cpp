#include "VertexArray.hpp"
#include <glad/glad.h>


VertexArray::~VertexArray()
{
	Destroy();
}

bool VertexArray::Create()
{
	Destroy();

	// Generate one VAO object ID;
	glGenVertexArrays(1, &m_ArrayID);
	return m_ArrayID != 0;
}

void VertexArray::Destroy()
{
	if (m_ArrayID != 0) {
		// Delete the VAO object
		glDeleteVertexArrays(1, &m_ArrayID);
		m_ArrayID = 0;
	}
}

void VertexArray::Bind() const
{
	// Make this VAO active
	glBindVertexArray(m_ArrayID);
}

void VertexArray::Unbind() const
{
	glBindVertexArray(0);
}

void VertexArray::SetAttribute(
	unsigned int index,
	int componentCount,
	unsigned int type,
	bool normalized,
	int stride,
	unsigned long long offset)
{
	// Defines how OpenGL should interpret vertex data for this attribute slot.
	// 
	// Example:
	// inde = 0
	// componentCount = 3
	// type = GL_FLOAT
	// stride = 3 * sizeof(float)
	// offset = 0
	// 
	// Means:
	// attribute 0 is a vec3 position pacaked tightly in the bound VBO.
	glVertexAttribPointer(
		index,
		componentCount,
		type,
		normalized ? GL_TRUE : GL_FALSE,
		stride,
		reinterpret_cast<const void*>(offset));

	// Enables this attribute slot so the vertex shader can read from it.
	glEnableVertexAttribArray(index);
}