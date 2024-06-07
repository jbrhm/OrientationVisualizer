#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define WEBGPU_CPP_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION // add this to exactly 1 of your C++ files

#include "Rendering.hpp"
#include "GLFW.hpp"

#include <thread>
#include <mutex>
#include <iostream>

using namespace std;

int main (int, char**) {
	GLFW::init();

	Rendering r;
	

	while(!r.shouldWindowClose()){


		r.render();

	}


	// This function call should be the very last thing inside main()
	GLFW::unInit();
	return 0;
}




