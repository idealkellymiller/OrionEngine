#pragma once


class VertexBuffer {
public:
	VertexBuffer() = default;
	~VertexBuffer();

	// Creates a GPU buffer and uploads vertex data into it.
	bool Create(const void* data, unsigned int sizeInBytes);

	// Deletes the GPU buffer if it exists.
	void Destroy();

	// Makes this vertex buffer the active GL_ARRAY_BUFFER.
	void Bind() const;

	// Unbind any GL_ARRAY_BUFFER
	void Unbind() const;

	unsigned int GetID() const { return m_BufferID; }
	bool IsValid() const { return m_BufferID != 0; }

private:
	unsigned int m_BufferID = 0;
};