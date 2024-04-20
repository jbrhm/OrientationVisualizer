// Define the flag that we are using webgpu_hpp
#include "GLFW/glfw3.h"
#include <memory>
#include <stdexcept>
#include <webgpu/webgpu_glfw.h>
#define WEBGPU_CPP_IMPLEMENTATION

//Tell the back end that we are using the dawn webgpu implementation
#define WEBGPU_BACKEND_DAWN

#include <limits>

// cpp Wrappers
// #include <webgpu.hpp>
#include "GLFW.cpp" //TODO: Fix LSP to work on header files and cpp files

using namespace wgpu;

const char* shaderSource = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
    var p = vec2f(0.0, 0.0);
    if (in_vertex_index == 0u) {
        p = vec2f(-0.5, -0.5);
    } else if (in_vertex_index == 1u) {
        p = vec2f(0.5, -0.5);
    } else {
        p = vec2f(0.0, 0.5);
    }
    return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

#ifdef WEBGPU_BACKEND_WGPU
shaderDesc.hintCount = 0;
shaderDesc.hints = nullptr;
#endif

class WebGPU {
private:

	// Create GLFW window
	GLFWwindow* window;
	void initWindow(){
		//TODO: Remove this creation of the window because it is scuffed
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // NEW
		window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
		// std::cout << "Requesting window..." << std::endl;
		// // This does nothing rn but might be needed later if params are needed
		// window = Window(640, 480, "Learn WebGPU");
		// std::cout << "Got window:" << std::endl;
	}

	// WebGPU Instance
	Instance instance;
	void initInstance(){
		std::cout << "Requesting instance..." << std::endl;
		InstanceDescriptor desc = {};
		desc.nextInChain = nullptr;
		instance = CreateInstance(&desc);
		if (!instance) {
			throw std::runtime_error("Could not initialize WebGPU!");
		}
		std::cout << "Got instance: " << std::endl;
	}

	// WeebGPU Adapter
	Adapter adapter;
	void initAdapter(){
		std::cout << "Requesting adapter..." << std::endl;
		RequestAdapterOptions opts = {};
		opts.nextInChain = nullptr;
		opts.compatibleSurface = surface;
		adapter = instance.requestAdapter(opts);
		std::cout << "Got adapter: " << adapter << std::endl;

		// TODO: No FeatureName defautl constructor defined
		std::vector<WGPUFeatureName> features;

		// Call the function a first time with a null return address, just to get
		// the entry count.
		size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);

		// Allocate memory (could be a new, or a malloc() if this were a C program)
		features.resize(featureCount);

		// Call the function a second time, with a non-null return address
		wgpuAdapterEnumerateFeatures(adapter, features.data());

		std::cout << "Adapter features:" << std::endl;
		for (auto f : features) {
			std::cout << " - " << f << std::endl;
		}
	}

	// WebGPU Surface
	Surface surface;
	void initSurface(){
		surface = glfw::CreateSurfaceForWindow(instance, window);
		// surface = glfwGetWGPUSurface(instance, window);
	}
	
	// WebGPU Device
	Device device;
	void initDevice(){
		std::cout << "Requesting device..." << std::endl;

		DeviceDescriptor deviceDesc = {};
		deviceDesc.nextInChain = nullptr;
		deviceDesc.label = "My Device"; // anything works here, that's your call
		deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
		deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
		deviceDesc.defaultQueue.nextInChain = nullptr;
		deviceDesc.defaultQueue.label = "The default queue";
		device = adapter.requestDevice(deviceDesc);

		std::cout << "Got device: " << device << std::endl;

		auto onDeviceError = [](WGPUErrorType type, char const* message) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
			std::cout << std::endl;
		};
		device.setUncapturedErrorCallback(onDeviceError);
	}

	// Device Queue
	Queue queue;
	void initQueue(){
		queue = device.getQueue();
		auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status) {
			std::cout << "Queued work finished with status: " << status << std::endl;
		};
		queue.onSubmittedWorkDone(onQueueWorkDone);
	}

	// Command Encoder
	CommandEncoder encoder;
	void initCommandEncoder(){
		CommandEncoderDescriptor encoderDesc = {};
		encoderDesc.nextInChain = nullptr;
		encoderDesc.label = "My command encoder";
		encoder = device.createCommandEncoder(encoderDesc);
		encoder.insertDebugMarker("Do one thing");
		encoder.insertDebugMarker("Do another thing");
	}
	
	// Create the swap chain
	SwapChain swapChain;
	void initSwapChain(){
		std::cout << "Creating Swapchain: " << std::endl;
		SwapChainDescriptor swapChainDesc = {};
		swapChainDesc.nextInChain = nullptr;
		swapChainDesc.width = 640;
		swapChainDesc.height = 480;
		swapChainDesc.format = WGPUTextureFormat_BGRA8Unorm;
		swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
		swapChainDesc.presentMode = WGPUPresentMode_Fifo;
		swapChain = device.createSwapChain(surface, swapChainDesc);
		std::cout << "Swapchain: " << swapChain << std::endl;
	}

	void presentNextTexture(){
		swapChain.present();
	}

	// Texture View
	// No member variable because this is a one time use variable
	std::shared_ptr<TextureView> getTextureView(){
		std::shared_ptr<TextureView> textureView = std::make_shared<TextureView>(swapChain.getCurrentTextureView());
		if (!textureView) {
			std::cerr << "Cannot acquire next swap chain texture" << std::endl;
			throw std::runtime_error("Cannot acquire next swap chain texture");
		}
		return textureView;
	}

	// Rendering Pipeline
	void initPipeline(){
		// Create the shader module from WGSL source code
		ShaderModuleDescriptor shaderDesc;
		ShaderModuleWGSLDescriptor shaderCodeDesc;
		// Set the chained struct's header
		shaderCodeDesc.chain.next = nullptr;
		shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
		// Connect the chain
		shaderDesc.nextInChain = &shaderCodeDesc.chain;
		ShaderModule shaderModule = device.createShaderModule(shaderDesc);

		RenderPipelineDescriptor pipelineDesc;

		// Configure the vertex fetch and vertex shader steps in the processing pipeline
		pipelineDesc.vertex.bufferCount = 0;
		pipelineDesc.vertex.buffers = nullptr;

		pipelineDesc.vertex.module = shaderModule;
		pipelineDesc.vertex.entryPoint = "vs_main";
		pipelineDesc.vertex.constantCount = 0;
		pipelineDesc.vertex.constants = nullptr;

		// Configure the assembly and rasterization stages
		// Each sequence of 3 vertices is considered as a triangle
		pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;

		// We'll see later how to specify the order in which vertices should be
		// connected. When not specified, vertices are considered sequentially.
		pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;

		// The face orientation is defined by assuming that when looking
		// from the front of the face, its corner vertices are enumerated
		// in the counter-clockwise (CCW) order.
		pipelineDesc.primitive.frontFace = FrontFace::CCW;

		// But the face orientation does not matter much because we do not
		// cull (i.e. "hide") the faces pointing away from us (which is often
		// used for optimization).
		pipelineDesc.primitive.cullMode = CullMode::None;

		// Configure the Fragment Shader
		FragmentState fragmentState;
		fragmentState.module = shaderModule;
		fragmentState.entryPoint = "fs_main";
		fragmentState.constantCount = 0;
		fragmentState.constants = nullptr;
		BlendState blendState;
		blendState.color.srcFactor = BlendFactor::SrcAlpha;
		blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
		blendState.color.operation = BlendOperation::Add;
		blendState.alpha.srcFactor = BlendFactor::Zero;
		blendState.alpha.dstFactor = BlendFactor::One;
		blendState.alpha.operation = BlendOperation::Add;
		ColorTargetState colorTarget;
		colorTarget.format = WGPUTextureFormat_BGRA8Unorm;
		colorTarget.blend = &blendState;
		colorTarget.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.

		// We have only one target because our render pass has only one output color
		// attachment.
		fragmentState.targetCount = 1;
		fragmentState.targets = &colorTarget;

		pipelineDesc.fragment = &fragmentState;
		pipelineDesc.depthStencil = nullptr;
		// Samples per pixel
		pipelineDesc.multisample.count = 1;
		// Default value for the mask, meaning "all bits on"
		pipelineDesc.multisample.mask = ~0u;
		// Default value as well (irrelevant for count = 1 anyways)
		pipelineDesc.multisample.alphaToCoverageEnabled = false;

		// We do not have any external resources for the pipeline
		pipelineDesc.layout = nullptr;

		device.createRenderPipeline(pipelineDesc);
	}
public:
	WebGPU(){
		initWindow();
		initInstance();
		initSurface();
		initAdapter();
		initDevice();
		initQueue();
		initCommandEncoder();
		initSwapChain();
	}

	~WebGPU(){
		glfwDestroyWindow(window);
	}

	void submitCommand(){
		CommandBufferDescriptor cmdBufferDescriptor = {};
		cmdBufferDescriptor.nextInChain = nullptr;
		cmdBufferDescriptor.label = "Command buffer";
		CommandBuffer command = encoder.finish(cmdBufferDescriptor);

		// Finally submit the command queue
		std::cout << "Submitting command..." << std::endl;
		queue.submit(command);
	}

	bool isWindowClosing(){
		//TODO: this is scuffed should use the RAII
		return glfwWindowShouldClose(window);
	}

	void pollEvents(){
		Window::pollEvents();
	}

	void renderColor(){
		// Get the texture where we will draw our next frame
		{
			std::shared_ptr<TextureView> textureView = getTextureView();
			std::cout << "nextTexture: " << textureView << std::endl;
			if (!textureView) {
				std::cerr << "Cannot acquire next swap chain texture" << std::endl;
				throw std::runtime_error("Cannot acquire next swap chain texture");
			}

			// Describe how the render pass will be performed
			RenderPassDescriptor renderPassDesc = {};

			// Attachments are the textures that will be rendered in the next rendering pass
			RenderPassColorAttachment renderPassColorAttachment = {};
			renderPassColorAttachment.view = *textureView;
			renderPassColorAttachment.resolveTarget = nullptr;
			renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
			renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
			renderPassColorAttachment.clearValue = Color{ 0.9, 0.1, 0.2, 1.0 };


			renderPassDesc.colorAttachmentCount = 1;
			// This is a ptr to only one thing right now because we are rendering only one frame
			renderPassDesc.colorAttachments = &renderPassColorAttachment; 
			renderPassDesc.depthStencilAttachment = nullptr;

			// These two parameters are used for measuring the time it takes to perform rendering steps 
			// bc it cant be done on the CPU
			renderPassDesc.timestampWrites = nullptr;

			renderPassDesc.nextInChain = nullptr;

			RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
			renderPass.end();
			textureView->release();
		}

		presentNextTexture();
	}
};
