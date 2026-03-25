#pragma once


class VertexArray {
public:
	VertexArray() = default;
	~VertexArray();

	bool Create();
	void Destroy();

	void Bind() const;
	void Unbind() const;

	// Configures a single vertex attribute in the currently bound VAO.
	void SetAttribute(
		unsigned int index,
		int componentCount,
		unsigned int type,
		bool normalized,
		int stride,
		unsigned long long offset);

	unsigned int GetID() const { return m_ArrayID; }
	bool IsValid() const { return m_ArrayID != 0; }

private:
	unsigned int m_ArrayID = 0;
};