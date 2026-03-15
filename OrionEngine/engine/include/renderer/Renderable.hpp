#pragma once
#include <glm/glm.hpp>


class Mesh;
class Material;


enum class RenderQueueType {
	Opaque,		// Draw first, can be sorted to redice state changes
	Transperant	// Draw later, usually back-to-front
};


struct Renderable {
	Mesh* MeshPtr = nullptr;					// Geometry to draw
	Material* MaterialPtr = nullptr;			// Appearance to use
	glm::mat4 ModelMatrix = glm::mat4(1.0f);	// Final world transform

	bool IsValid() const {
		// A renerable is drawable only of it has both a mesh and a material
		return MeshPtr != nullptr && MaterialPtr != nullptr;
	}
};