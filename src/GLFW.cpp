#include "GLFW.hpp"

using namespace std;

bool GLFW::isGLFWInit = false;

void GLFW::glfwErrorCallback(int error, const char* description){
    printf("GLFW Error %d: %s\n", error, description);
}

vector<GLFWwindow*> GLFW::windows = vector<GLFWwindow*>();

void GLFW::init(){
	if(!isGLFWInit){
		glfwSetErrorCallback(glfwErrorCallback);
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		isGLFWInit = true;
	}
}

void GLFW::unInit(){
	// Destroy all of the windows in the vector
	for(GLFWwindow* ptr : windows){
		glfwDestroyWindow(ptr);
	}

	// Terminate GLFW
	glfwTerminate();
}

GLFWwindow* GLFW::windowFactory(int width, int height, const string& name){
	windows.push_back(glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr));
	return windows.at(windows.size() - 1);
}

bool GLFW::shouldWindowClose(GLFWwindow* ptr){
	return glfwWindowShouldClose(ptr);
}
