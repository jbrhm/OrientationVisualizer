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

	void setupWindowSettings(){
		// Since we are managing the graphics behind the scenes with WebGPU GLFW should not use its graphics api
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	}

public:
	Window(int width, int height, const std::string& name) : mWidth(width), mHeight(height), mName(name){
		setupWindowSettings();
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

	GLFWwindow* getWindowPtr(){
		return window;
	}

	// Static functions
	static void pollEvents(){
		if(refCount){
			glfwPollEvents();
		}
	}

	static bool isGLFWInit(){
		return isInit;
	}
};

//Init Statics
