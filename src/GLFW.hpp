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

	static std::unique_ptr<std::vector<std::shared_ptr<GLFWwindow*>>> windows;


public:
	static void init();

	static void unInit();

	static std::shared_ptr<GLFWwindow*> windowFactory(int width, int height, const std::string& name);

	static bool shouldWindowClose(std::shared_ptr<GLFWwindow*> ptr);
};
