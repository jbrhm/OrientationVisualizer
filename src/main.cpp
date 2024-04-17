// Define the flag that we are using webgpu_hpp
#define WEBGPU_CPP_IMPLEMENTATION

#include <iostream>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>


int main (int, char**){
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

	//Create te WebGPU instance
	WGPUInstance instance = wgpuCreateInstance(&desc);
	std::cout << "Instance: " << instance << std::endl;

	// Get the wgpu adapter
	//WGPURequestAdapterOptions adapterOpts = {};
	//WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);

	// Destroy the WebGPU instance
	wgpuInstanceRelease(instance);
	return 0;
}
