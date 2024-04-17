#include <iostream>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>

int main (int, char**){
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

	WGPUInstance instance = wgpuCreateInstance(&desc);
	std::cout << "Instance: " << instance << std::endl;
	return 0;
}
