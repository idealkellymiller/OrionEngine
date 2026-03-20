#pragma once
#include <glm/glm.hpp>


class Mesh;
class Material;


//enum class RenderQueueType {
//	Opaque,		// Draw first, can be sorted to redice state changes
//	Transperant	// Draw later, usually back-to-front
//};


struct Renderable {
	Mesh* MeshPtr = nullptr;					// Geometry to draw
	Material* MaterialPtr = nullptr;			// Appearance to use
	glm::mat4 ModelMatrix = glm::mat4(1.0f);	// Final world transform

	// Optional future-facing fields:
	// Layer or render group lets you filter objects later
	// without touching the ECS.
	unsigned int RenderLayer = 0;

	// usefule for skipping hidden editor/debug objects.
	bool Visible = true;

	bool IsValid() const {
		// A renerable is drawable only of it has both a mesh and a material
		return Visible && MeshPtr != nullptr && MaterialPtr != nullptr;
	}
};