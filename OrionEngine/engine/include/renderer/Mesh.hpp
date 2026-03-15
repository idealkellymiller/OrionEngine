#pragma once
#include <vector>

#include "Vertex.hpp"
#include "VertexArray.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "BoundingSphere.hpp"


class Mesh
{
public:
    Mesh();
    ~Mesh();

    // Non-copyable for now because it owns GPU resources.
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Create from vertices only (fallback path)
    // bool Create(const std::vector<Vertex>& vertices);
    // Create from vertices and indices (preferred path)
    bool Create(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    // Destroy GPU resources.
    void Destroy();

    bool IsValid() const;

    int GetVertexCount() const { return m_VertexCount; }
    int GetIndexCount() const { return m_IndexCount; }
    bool HasIndices() const { return m_IndexCount > 0; }

    VertexArray& GetVertexArray();
    const VertexArray& GetVertexArray() const;

    const BoundingSphere& GetBounds() const { return m_Bounds; }


private:
    // Computes local-space bounding sphere from vertex positions
    void ComputeBounds(const std::vector<Vertex>& vertices);

private:
    VertexArray* m_VertexArray = nullptr;
    VertexBuffer* m_VertexBuffer = nullptr;
    IndexBuffer* m_IndexBuffer = nullptr;

    int m_VertexCount = 0;
    int m_IndexCount = 0;

    BoundingSphere m_Bounds;
};
