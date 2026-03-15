#include "Mesh.hpp"
#include "Vertex.hpp"
#include "VertexArray.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"

#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <algorithm>


Mesh::Mesh()
{
	m_VertexArray = new VertexArray();
	m_VertexBuffer = new VertexBuffer();
	m_IndexBuffer = new IndexBuffer();
}

Mesh::~Mesh()
{
	Destroy();

	delete m_VertexArray;
	delete m_VertexBuffer;
	delete m_IndexBuffer;

	m_VertexArray = nullptr;
	m_VertexBuffer = nullptr;
	m_IndexBuffer = nullptr;
}
/*
bool Mesh::Create(const std::vector<Vertex>& vertices)
{
	std::vector<unsigned int> emptyIndices;
	return Create(vertices, emptyIndices);
}
*/
bool Mesh::Create(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	Destroy();

	if (vertices.empty()) {
		return false;
	}
	
	m_VertexCount = static_cast<int>(vertices.size());
	m_IndexCount = static_cast<int>(indices.size());

	// Copmuter local-space bounds from vertex positions
	ComputeBounds(vertices);

	if (!m_VertexArray->Create()) {
		printf("couldn't make vertex array\n");
		return false;
	}

	if (!m_VertexBuffer->Create(vertices.data(), static_cast<unsigned int>(vertices.size() * sizeof(Vertex)))) {
		printf("couldn't make Vertex buffer\n");
		return false;
	}


	// Bind VAO first so vertex attribute setup and element buffer binding are captured by it.
	m_VertexArray->Bind();
	m_VertexBuffer->Bind();

	// If indices are provided, create and bind index buffer while VAO is bound.
	if (!indices.empty()) {
		if (!m_IndexBuffer->Create(indices.data(), static_cast<unsigned int>(indices.size()))) {
			printf("couldn't make Index buffer\n");
			return false;
		}

		m_IndexBuffer->Bind();
	}


	// Attribute 0 = Position (vec3)
	m_VertexArray->SetAttribute(
		0, 3, GL_FLOAT, false,
		sizeof(Vertex),
		offsetof(Vertex, Position));

	// Attribute 1 = Normal (vec3)
	m_VertexArray->SetAttribute(
		1, 3, GL_FLOAT, false,
		sizeof(Vertex),
		offsetof(Vertex, Normal));

	// Attribute 2 = UV (vec2)
	m_VertexArray->SetAttribute(
		2, 2, GL_FLOAT, false,
		sizeof(Vertex),
		offsetof(Vertex, UV));

	m_VertexBuffer->Unbind();

	// Leave the index buffer association captured by the VAO.
	m_VertexArray->Unbind();

	return true;
}

void Mesh::ComputeBounds(const std::vector<Vertex>& vertices)
{
	if (vertices.empty()) {
		m_Bounds.Center = glm::vec3(0.0f);
		m_Bounds.Radius = 0.0f;
		return;
	}

	// Estimate center by everaging all vertex positions.
	glm::vec3 center(0.0f);
	for (const Vertex& v : vertices)
	{
		center += v.Position;
	}
	center /= static_cast<float>(vertices.size());

	// Find the farthest vertex from that center
	float maxDistanceSq = 0.0f;
	for (const Vertex& v : vertices)
	{
		glm::vec3 offset = v.Position - center;
		float distSq = glm::dot(offset, offset);
		if (distSq > maxDistanceSq)
		{
			maxDistanceSq = distSq;
		}
	}

	m_Bounds.Center = center;
	m_Bounds.Radius = sqrtf(maxDistanceSq);
}

void Mesh::Destroy()
{
	if (m_IndexBuffer)
		m_IndexBuffer->Destroy();

	if (m_VertexBuffer)
		m_VertexBuffer->Destroy();

	if (m_VertexArray)
		m_VertexArray->Destroy();

	m_VertexCount = 0;
	m_IndexCount = 0;
}

bool Mesh::IsValid() const
{
	if (m_VertexArray == nullptr || m_VertexBuffer == nullptr)
		return false;

	if (!m_VertexArray->IsValid() || !m_VertexBuffer->IsValid())
		return false;

	// If mesh says it has indices, the index buffer must be valid too.
	if (m_IndexCount > 0)
	{
		if (m_IndexBuffer == nullptr || !m_IndexBuffer->IsValid())
			return false;
	}

	return m_VertexCount > 0;
}

VertexArray& Mesh::GetVertexArray()
{
	return *m_VertexArray;
}

const VertexArray& Mesh::GetVertexArray() const
{
	return *m_VertexArray;
}