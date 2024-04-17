// Define the flag that we are using webgpu_hpp
#define WEBGPU_CPP_IMPLEMENTATION

#include <iostream>
#include <GLFW/glfw3.h>
#include <limits>
#include <webgpu.hpp>

using namespace wgpu;

int main (int, char**){

	BufferDescriptor deviceDesc = {};
	Device device;
	device.createBuffer(deviceDesc);

	RenderPipelineDescriptor pipelineDesc;
	device.createRenderPipeline(pipelineDesc);
	return 0;
}
