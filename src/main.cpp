#include <iostream>
#include <limits>
#include "WebGPU.cpp"

int main (int, char**){
	WebGPU webgpu;
	webgpu.submitCommand();

	while(!webgpu.isWindowClosing()){
		webgpu.pollEvents();
	}

	return 0;
}
