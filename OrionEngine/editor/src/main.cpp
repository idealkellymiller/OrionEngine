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


	// 1) Renderer Initialization
	Renderer::Init();


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