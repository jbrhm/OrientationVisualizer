#include <iostream>
#include <limits>
#include "WebGPU.cpp"

int main (int, char**){
	WebGPU webgpu;
	webgpu.submitCommand();

	while(!webgpu.isWindowClosing()){
		webgpu.pollEvents();
		webgpu.renderColor();
	}

	return 0;
}
