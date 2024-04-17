#include <cstdlib>
#include <iostream>
#include <GLFW/glfw3.h>

const uint32_t kWidth = 512;
const uint32_t kHeight = 512;

int main (int, char**){
	//Initialize GLFW and make sure it initializes properly
	if(!glfwInit()){
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Learn WebGPU", nullptr, nullptr);

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

	glfwTerminate();
	return 0;
}
