#include "GLFW.hpp"

using namespace std;

bool GLFW::isGLFWInit = false;

unique_ptr<vector<shared_ptr<GLFWwindow*>>> GLFW::windows = make_unique<vector<shared_ptr<GLFWwindow*>>>();

void GLFW::init(){
	if(!isGLFWInit){
		glfwInit();
		isGLFWInit = true;
	}
}

void GLFW::unInit(){
	windows.reset();
	glfwTerminate();
}

shared_ptr<GLFWwindow*> GLFW::windowFactory(int width, int height, const string& name){
	windows->push_back(make_shared<GLFWwindow*>(glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr)));
	return windows->at(windows->size() - 1);
}



bool GLFW::shouldWindowClose(std::shared_ptr<GLFWwindow*> ptr){
	return glfwWindowShouldClose(*ptr);
}

template <GLFWwindow*>
std::shared_ptr<GLFWwindow*> make_shared(GLFWwindow* ptr) {
    return std::shared_ptr<GLFWwindow*>(ptr, WindowDestructor()); // This is correct
}

// Custom destructor for std::shared_ptrs of GLFWwindow
void WindowDestructor::operator()(GLFWwindow** window){
	glfwDestroyWindow(*window);
}
