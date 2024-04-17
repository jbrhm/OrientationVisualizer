#include <cstdlib>
#include <iostream>
#include <GLFW/glfw3.h>

int main (int, char**){
	//Initialize GLFW and make sure it initializes properly
	if(!glfwInit()){
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return EXIT_FAILURE;
	}

	GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);

	if (!window) {
		std::cerr << "Could not open window!" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}

	// Make sure the window is open
	while (!glfwWindowShouldClose(window)) {
		// Check whether the user clicked on the close button (and any other
		// mouse/key event, which we don't use so far)
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	std::cout << "lol" << std::endl;
	glfwTerminate();
	return 0;
}
