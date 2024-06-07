#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include <string>

struct WindowDestructor{
	void operator()(GLFWwindow** window);
};

class GLFW {
private:
	static bool isGLFWInit;

	static std::vector<GLFWwindow*> windows;

	static void glfwErrorCallback(int error, const char* description);

public:
	static void init();

	static void unInit();

	static GLFWwindow* windowFactory(int width, int height, const std::string& name);

	static bool shouldWindowClose(GLFWwindow* ptr);
};
