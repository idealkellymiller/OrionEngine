#include <Orion.h>

#include "GLFW/glfw3.h"


int main() {
	// 0) GLFW Window Initialization
	if (!glfwInit())
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 600, "Orion Editor", nullptr, nullptr);

	if (!window) {
		glfwTerminate();
		return -1;
	}

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f); // RGBA
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	glViewport(0, 0, w, h);


	// 1) Renderer Initialization
	Renderer::Init();
	
	// Create a triangle (mesh) object
	Mesh* triangle = new Mesh();




	// 2) Main Loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();


	}


	// 3) Shutdown
    Renderer::Shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();
    return 0;
}