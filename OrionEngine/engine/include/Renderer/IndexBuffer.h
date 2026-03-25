#pragma once


class IndexBuffer {
public:
    IndexBuffer() = default;
    ~IndexBuffer();

    // Creates an element/index buffer and uploads index data.
    bool Create(const unsigned int* data, unsigned int count);

    void Destroy();

    void Bind() const;
    void Unbind() const;

    unsigned int GetID() const { return m_BufferID; }
    unsigned int GetCount() const { return m_Count; }
    bool IsValid() const { return m_BufferID != 0 && m_Count > 0; }

private:
    unsigned int m_BufferID = 0;
    unsigned int m_Count = 0;
};
