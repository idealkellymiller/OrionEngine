#pragma once


class Mesh {
public:
	Mesh();
	void Draw();
	~Mesh();

private:
	// Vertex Buffer Object, Vertex Array Object
	unsigned int VBO, VAO, vertex_count;
};