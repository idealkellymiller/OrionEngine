#pragma once
#include "external/glad/glad.h"


class Mesh {
public:
	Mesh();
	void Draw();
	~Mesh();

private:
	// Vertex Buffer Object, Vertex Array Object
	unsigned int VBO, VAO, vertex_count;
};