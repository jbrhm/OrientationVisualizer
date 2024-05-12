#include "Rendering.hpp"

using namespace wgpu;
using glm::mat4x4;
using glm::vec4;
using glm::vec3;

void Rendering::initInstance(){
    mInstance = createInstance(InstanceDescriptor{});   
    if (!mInstance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		throw std::runtime_error("Could not initialize WebGPU!");
	}
}

void Rendering::initGLFW(){
    if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		throw std::runtime_error("Could not initialize GLFW!");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	mWindow = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
	if (!mWindow) {
		std::cerr << "Could not open mWindow!" << std::endl;
		throw std::runtime_error("Could not open mWindow!");
	}
}

void Rendering::initAdapter(){
    std::cout << "Requesting adapter..." << std::endl;
	mSurface = glfwGetWGPUSurface(mInstance, mWindow);
	RequestAdapterOptions adapterOpts{};
	adapterOpts.compatibleSurface = mSurface;
	mAdapter = mInstance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << mAdapter << std::endl;

	mSupportedLimits = SupportedLimits{};
	mAdapter.getLimits(&mSupportedLimits);
}

void Rendering::initDevice(){
    std::cout << "Requesting device..." << std::endl;
	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 3;
	requiredLimits.limits.maxVertexBuffers = 1;
	// Update max buffer size to allow up to 10000 vertices in the loaded file:
	requiredLimits.limits.maxBufferSize = 10000 * sizeof(VertexAttributes);
	requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
	requiredLimits.limits.minStorageBufferOffsetAlignment = mSupportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.minUniformBufferOffsetAlignment = mSupportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.maxInterStageShaderComponents = 6;
	requiredLimits.limits.maxBindGroups = 1;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
	requiredLimits.limits.maxTextureDimension1D = 480;
	requiredLimits.limits.maxTextureDimension2D = 640;
	requiredLimits.limits.maxTextureArrayLayers = 1;

	DeviceDescriptor deviceDesc;
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.label = "The default queue";
	mDevice = mAdapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << mDevice << std::endl;

	// Add an error callback for more debug info
	auto h = mDevice.setUncapturedErrorCallback([](ErrorType type, char const* message) {
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl;
	});
}

void Rendering::initQueue(){
	std::cout << "Creating Queue..." << std::endl;
	mQueue = mDevice.getQueue();
}

void Rendering::initSwapChain(){
	std::cout << "Creating swapchain..." << std::endl;
	SwapChainDescriptor swapChainDesc;
	swapChainDesc.width = 640;
	swapChainDesc.height = 480;
	swapChainDesc.usage = TextureUsage::RenderAttachment;
	swapChainDesc.format = mSwapChainFormat;
	swapChainDesc.presentMode = PresentMode::Fifo;
	mSwapChain = mDevice.createSwapChain(mSurface, swapChainDesc);
	std::cout << "Swapchain: " << mSwapChain << std::endl;
}

void Rendering::initShaderModule(){
	std::cout << "Creating shader module..." << std::endl;
	mShaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", mDevice);
	std::cout << "Shader module: " << mShaderModule << std::endl;
}

void Rendering::initRenderPipeline(){
	std::cout << "Creating render pipeline..." << std::endl;
	RenderPipelineDescriptor pipelineDesc;

	// Vertex fetch
	mVertexAttribs = std::vector<wgpu::VertexAttribute>(3);
	//                                         ^ This was a 2

	// Position attribute
	mVertexAttribs[0].shaderLocation = 0;
	mVertexAttribs[0].format = VertexFormat::Float32x3;
	mVertexAttribs[0].offset = 0;

	// Normal attribute
	mVertexAttribs[1].shaderLocation = 1;
	mVertexAttribs[1].format = VertexFormat::Float32x3;
	mVertexAttribs[1].offset = offsetof(VertexAttributes, normal);

	// Color attribute
	mVertexAttribs[2].shaderLocation = 2;
	mVertexAttribs[2].format = VertexFormat::Float32x3;
	mVertexAttribs[2].offset = offsetof(VertexAttributes, color);

	mVertexBufferLayout.attributeCount = (uint32_t)mVertexAttribs.size();
	mVertexBufferLayout.attributes = mVertexAttribs.data();
	mVertexBufferLayout.arrayStride = sizeof(VertexAttributes);
	//                               ^^^^^^^^^^^^^^^^^^^^^^^^ This was 6 * sizeof(float)
	mVertexBufferLayout.stepMode = VertexStepMode::Vertex;

	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &mVertexBufferLayout;

	pipelineDesc.vertex.module = mShaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDesc.primitive.frontFace = FrontFace::CCW;
	pipelineDesc.primitive.cullMode = CullMode::None;

	
	pipelineDesc.fragment = &mFragmentState;
	mFragmentState.module = mShaderModule;
	mFragmentState.entryPoint = "fs_main";
	mFragmentState.constantCount = 0;
	mFragmentState.constants = nullptr;

	
	mBlendState.color.srcFactor = BlendFactor::SrcAlpha;
	mBlendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
	mBlendState.color.operation = BlendOperation::Add;
	mBlendState.alpha.srcFactor = BlendFactor::Zero;
	mBlendState.alpha.dstFactor = BlendFactor::One;
	mBlendState.alpha.operation = BlendOperation::Add;

	
	mColorTarget.format = mSwapChainFormat;
	mColorTarget.blend = &mBlendState;
	mColorTarget.writeMask = ColorWriteMask::All;

	mFragmentState.targetCount = 1;
	mFragmentState.targets = &mColorTarget;

	mDepthStencilState = Default;
	mDepthStencilState.depthCompare = CompareFunction::Less;
	mDepthStencilState.depthWriteEnabled = true;
	
	mDepthStencilState.format = mDepthTextureFormat;
	mDepthStencilState.stencilReadMask = 0;
	mDepthStencilState.stencilWriteMask = 0;
	
	pipelineDesc.depthStencil = &mDepthStencilState;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Create binding layout (don't forget to = Default)
	mBindingLayout = Default;
	mBindingLayout.binding = 0;
	mBindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
	mBindingLayout.buffer.type = BufferBindingType::Uniform;
	mBindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

	// Create a bind group layout
	BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.entryCount = bindGroupLayoutDescEntryCount;
	bindGroupLayoutDesc.entries = &mBindingLayout;
	mBindGroupLayout = mDevice.createBindGroupLayout(bindGroupLayoutDesc);

	// Create the pipeline layout
	PipelineLayoutDescriptor layoutDesc{};
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&mBindGroupLayout;
	mPipelinelayout = mDevice.createPipelineLayout(layoutDesc);
	pipelineDesc.layout = mPipelinelayout;

	mPipeline = mDevice.createRenderPipeline(pipelineDesc);
	std::cout << "Render pipeline: " << mPipeline << std::endl;
}

void Rendering::initTexture(){
	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = mDepthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = {640, 480, 1};
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat*)&mDepthTextureFormat;
	mDepthTexture = mDevice.createTexture(depthTextureDesc);
	std::cout << "Depth texture: " << mDepthTexture << std::endl;
}

void Rendering::initTextureView(){
	// Create the view of the depth texture manipulated by the rasterizer
	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::_2D;
	depthTextureViewDesc.format = mDepthTextureFormat;
	mDepthTextureView = mDepthTexture.createView(depthTextureViewDesc);
	std::cout << "Depth texture view: " << mDepthTextureView << std::endl;
}

void Rendering::loadGeometry(){
	bool success = loadGeometryFromObj(RESOURCE_DIR "/pyramid.obj", mVertexData);
	if (!success) {
		std::cerr << "Could not load geometry!" << std::endl;
		throw std::runtime_error("Could not load geometry!");
	}
}

void Rendering::initVertexBuffer(){
	// Create vertex buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = mVertexData.size() * sizeof(VertexAttributes); // changed
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	mVertexBuffer = mDevice.createBuffer(bufferDesc);
	mQueue.writeBuffer(mVertexBuffer, 0, mVertexData.data(), bufferDesc.size); // changed

	mIndexCount = static_cast<int>(mVertexData.size()); // changed
}

void Rendering::initUniformBuffer(){
	BufferDescriptor bufferDesc;
	bufferDesc.size = mVertexData.size() * sizeof(VertexAttributes); // changed
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	// Create uniform buffer
	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	mUniformBuffer = mDevice.createBuffer(bufferDesc);
}

void Rendering::initUniforms(){
	// Build transform matrices

	// Translate the view
	vec3 focalPoint(0.0, 0.0, -1.0);
	// Rotate the object
	angle1 = 2.0f; // arbitrary time
	// Rotate the view point
	angle2 = 3.0f * PI / 4.0f;

	S = glm::scale(mat4x4(1.0), vec3(0.3f));
	T1 = mat4x4(1.0);
	R1 = glm::rotate(mat4x4(1.0), angle1, vec3(0.0, 0.0, 1.0));
	mUniforms.modelMatrix = R1 * T1 * S;

	mat4x4 R2 = glm::rotate(mat4x4(1.0), -angle2, vec3(1.0, 0.0, 0.0));
	mat4x4 T2 = glm::translate(mat4x4(1.0), -focalPoint);
	mUniforms.viewMatrix = T2 * R2;

	float ratio = 640.0f / 480.0f;
	float focalLength = 2.0;
	float near = 0.01f;
	float far = 100.0f;
	float divider = 1 / (focalLength * (far - near));
	mUniforms.projectionMatrix = transpose(mat4x4(
		1.0, 0.0, 0.0, 0.0,
		0.0, ratio, 0.0, 0.0,
		0.0, 0.0, far * divider, -far * near * divider,
		0.0, 0.0, 1.0 / focalLength, 0.0
	));

	mUniforms.time = 1.0f;
	mUniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	mQueue.writeBuffer(mUniformBuffer, 0, &mUniforms, sizeof(MyUniforms));
}

void Rendering::initBinding(){
	// Create a binding
	mBinding.binding = 0;
	mBinding.buffer = mUniformBuffer;
	mBinding.offset = 0;
	mBinding.size = sizeof(MyUniforms);
}

void Rendering::initBindGroup(){
	// A bind group contains one or multiple bindings
	BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = mBindGroupLayout;
	bindGroupDesc.entryCount = bindGroupLayoutDescEntryCount;
	bindGroupDesc.entries = &mBinding;
	mBindGroup = mDevice.createBindGroup(bindGroupDesc);
}

bool Rendering::shouldWindowClose(){
	return glfwWindowShouldClose(mWindow);
}

void Rendering::render(){
	glfwPollEvents();

	// Update uniform buffer
	mUniforms.time = static_cast<float>(glfwGetTime()); // glfwGetTime returns a double
	// Only update the 1-st float of the buffer
	mQueue.writeBuffer(mUniformBuffer, offsetof(MyUniforms, time), &mUniforms.time, sizeof(MyUniforms::time));

	// Update view matrix
	angle1 = mUniforms.time;
	R1 = glm::rotate(mat4x4(1.0), angle1, vec3(0.0, 0.0, 1.0));
	mUniforms.modelMatrix = R1 * T1 * S;
	mQueue.writeBuffer(mUniformBuffer, offsetof(MyUniforms, modelMatrix), &mUniforms.modelMatrix, sizeof(MyUniforms::modelMatrix));

	TextureView nextTexture = mSwapChain.getCurrentTextureView();
	if (!nextTexture) {
		std::cerr << "Cannot acquire next swap chain texture" << std::endl;
		throw std::runtime_error("Cannot acquire next swap chain texture");
	}

	CommandEncoderDescriptor commandEncoderDesc;
	commandEncoderDesc.label = "Command Encoder";
	CommandEncoder encoder = mDevice.createCommandEncoder(commandEncoderDesc);
	
	RenderPassDescriptor renderPassDesc{};

	RenderPassColorAttachment renderPassColorAttachment{};
	renderPassColorAttachment.view = nextTexture;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = LoadOp::Clear;
	renderPassColorAttachment.storeOp = StoreOp::Store;
	renderPassColorAttachment.clearValue = Color{ 0.05, 0.05, 0.05, 1.0 };
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	RenderPassDepthStencilAttachment depthStencilAttachment;
	depthStencilAttachment.view = mDepthTextureView;
	depthStencilAttachment.depthClearValue = 1.0f;
	depthStencilAttachment.depthLoadOp = LoadOp::Clear;
	depthStencilAttachment.depthStoreOp = StoreOp::Store;
	depthStencilAttachment.depthReadOnly = false;
	depthStencilAttachment.stencilClearValue = 0;
#ifdef WEBGPU_BACKEND_WGPU
	depthStencilAttachment.stencilLoadOp = LoadOp::Clear;
	depthStencilAttachment.stencilStoreOp = StoreOp::Store;
#else
	depthStencilAttachment.stencilLoadOp = LoadOp::Undefined;
	depthStencilAttachment.stencilStoreOp = StoreOp::Undefined;
#endif
	depthStencilAttachment.stencilReadOnly = true;

	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

	renderPassDesc.timestampWriteCount = 0;
	renderPassDesc.timestampWrites = nullptr;
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

	renderPass.setPipeline(mPipeline);

	renderPass.setVertexBuffer(0, mVertexBuffer, 0, mVertexData.size() * sizeof(VertexAttributes)); // changed

	// Set binding group
	renderPass.setBindGroup(0, mBindGroup, 0, nullptr);

	renderPass.draw(mIndexCount, 1, 0, 0); // changed

	renderPass.end();
	renderPass.release();
	
	nextTexture.release();

	CommandBufferDescriptor cmdBufferDescriptor{};
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.finish(cmdBufferDescriptor);
	encoder.release();
	mQueue.submit(command);
	command.release();

	mSwapChain.present();

#ifdef WEBGPU_BACKEND_DAWN
	// TODO: Figure out why this has to be wrapped in ifdefs bc it is defined :(
	device.tick();
#endif
}

Rendering::Rendering(){
    static_assert(sizeof(MyUniforms) % 16 == 0);

	initInstance();

	initGLFW();

	initAdapter();

	initDevice();

	initQueue();

	initSwapChain();

	initShaderModule();

	initRenderPipeline();

	initTexture();

	initTextureView();

	loadGeometry();

	initVertexBuffer();

	initUniformBuffer();
	
	initUniforms();
	
	initBinding();

	initBindGroup();
	

	

	
}

bool Rendering::loadGeometryFromObj(const std::filesystem::path& path, std::vector<VertexAttributes>& mVertexData) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	// Call the core loading procedure of TinyOBJLoader
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str());

	// Check errors
	if (!warn.empty()) {
		std::cout << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		return false;
	}

	// Filling in mVertexData:
	mVertexData.clear();
	for (const auto& shape : shapes) {
		size_t offset = mVertexData.size();
		mVertexData.resize(offset + shape.mesh.indices.size());

		for (size_t i = 0; i < shape.mesh.indices.size(); ++i) {
			const tinyobj::index_t& idx = shape.mesh.indices[i];

			mVertexData[offset + i].position = {
				attrib.vertices[3 * idx.vertex_index + 0],
				-attrib.vertices[3 * idx.vertex_index + 2], // Add a minus to avoid mirroring
				attrib.vertices[3 * idx.vertex_index + 1]
			};

			// Also apply the transform to normals!!
			mVertexData[offset + i].normal = {
				attrib.normals[3 * idx.normal_index + 0],
				-attrib.normals[3 * idx.normal_index + 2],
				attrib.normals[3 * idx.normal_index + 1]
			};

			mVertexData[offset + i].color = {
				attrib.colors[3 * idx.vertex_index + 0],
				attrib.colors[3 * idx.vertex_index + 1],
				attrib.colors[3 * idx.vertex_index + 2]
			};
		}
	}

	return true;
}

ShaderModule Rendering::loadShaderModule(const std::filesystem::path& path, Device device) {
	std::ifstream file(path);
	if (!file.is_open()) {
		return nullptr;
	}
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	std::string shaderSource(size, ' ');
	file.seekg(0);
	file.read(shaderSource.data(), size);

	ShaderModuleWGSLDescriptor shaderCodeDesc;
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
	shaderCodeDesc.code = shaderSource.c_str();
	ShaderModuleDescriptor shaderDesc;
	shaderDesc.nextInChain = &shaderCodeDesc.chain;
#ifdef WEBGPU_BACKEND_WGPU
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;
#endif

	return device.createShaderModule(shaderDesc);
}

Rendering::~Rendering(){
	mVertexBuffer.destroy();
	mVertexBuffer.release();

	// Destroy the depth texture and its view
	mDepthTextureView.release();
	mDepthTexture.destroy();
	mDepthTexture.release();

	mPipeline.release();
	mShaderModule.release();
	mSwapChain.release();
	mDevice.release();
	mAdapter.release();
	mInstance.release();
	mSurface.release();
	glfwDestroyWindow(mWindow);
	glfwTerminate();
}