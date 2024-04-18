#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <iostream>
#include <stdexcept>
#include <string>


// This is a wrapper class for RAII on GLFW windows

class Window{
private:
	static inline bool isInit = false;
	static inline int refCount = 0;
	GLFWwindow* window;

	// Size
	int mWidth;
	int mHeight;

	// Name
	std::string mName;

public:

	static void pollEvents(){
		if(refCount){
			glfwPollEvents();
		}
	}

	static bool isGLFWInit(){
		return isInit;
	}

	Window(int width, int height, const std::string& name) : mWidth(width), mHeight(height), mName(name){
		if(!isInit){
			if (!glfwInit()) {
				std::cerr << "Could not initialize GLFW!" << std::endl;
				throw std::runtime_error("GLFW didn't initialize properly");
			}
		}

		// Create the window pointer
		window = glfwCreateWindow(mWidth, mHeight, mName.c_str(), NULL, NULL);

		// Increment the refcount so we know when to un-init GLFW
		refCount++;
	}

	~Window(){
		// Release the window and update the refcount
		glfwDestroyWindow(window);
		refCount--;

		if(!refCount){
			glfwTerminate();
		}
	}
};

//Init Statics
