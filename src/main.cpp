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
	constexpr bool isDebug = false;

	Rendering r;
	std::mutex renderingMutex{};

	auto inputFunction = [&](){
		while(!r.shouldWindowClose()){

			r.getNewInput();

			renderingMutex.lock();

			if constexpr (isDebug){
				std::cout << "Input Locked" << std::endl;
			}

			r.writeRotation();

			renderingMutex.unlock();

			if constexpr (isDebug){
				std::cout << "Input Unlocked" << std::endl;
			}
		}
	};

	auto renderFunction = [&](){
		while(!r.shouldWindowClose()){
			renderingMutex.lock();

			if constexpr (isDebug){
				std::cout << "Render Locked" << std::endl;
			}

			r.render();

			renderingMutex.unlock();

			if constexpr (isDebug){
				std::cout << "Render Unlocked" << std::endl;
			}
		}
	};

	std::thread renderingThread(renderFunction);
	std::thread inputThread(inputFunction);

	renderingThread.join();
	inputThread.join();

	// This function call should be the very last thing inside main()
	GLFW::unInit();
	return 0;
}




