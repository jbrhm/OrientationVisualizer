#include "Rendering.hpp"
#include <assimp-3.3.1/include/assimp/Importer.hpp>
#include <assimp-3.3.1/include/assimp/cimport.h>
#include <assimp-3.3.1/include/assimp/postprocess.h>
#include <assimp-3.3.1/include/assimp/scene.h>
#include <assimp-3.3.1/include/assimp/material.h>

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
	mWindow = GLFW::windowFactory(WINDOW_WIDTH, WINDOW_HEIGHT, "Visualizer");
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
	requiredLimits.limits.maxBufferSize = MAX_BUFFER_SIZE;
	requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
	requiredLimits.limits.minStorageBufferOffsetAlignment = mSupportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.minUniformBufferOffsetAlignment = mSupportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.maxInterStageShaderComponents = 6;
	requiredLimits.limits.maxBindGroups = 1;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	requiredLimits.limits.maxUniformBufferBindingSize = sizeof(MyUniforms);
	requiredLimits.limits.maxTextureDimension1D = WINDOW_HEIGHT;
	requiredLimits.limits.maxTextureDimension2D = WINDOW_WIDTH;
	requiredLimits.limits.maxTextureArrayLayers = 1;
	requiredLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;

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
	swapChainDesc.width = WINDOW_WIDTH;
	swapChainDesc.height = WINDOW_HEIGHT;
	swapChainDesc.usage = TextureUsage::RenderAttachment;
	swapChainDesc.format = mSwapChainFormat;
	swapChainDesc.presentMode = PresentMode::Mailbox; // Prevents Queue from being overloaded only (keeps one)
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
	// Dynamic offsets allow the render pipeline to use different parts of the buffer on different draws 
	mBindingLayout.buffer.hasDynamicOffset = true;

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

void Rendering::getNewInput(){
	double q0 = 0;
	double q1 = 0;
	double q2 = 0;
	double q3 = 0;
	std::cin >> q3; // Not sure why these have to be backwards
	std::cin >> q2;
	std::cin >> q1;
	std::cin >> q0;

	std::cout << "The Quaternion you entered was: " << q3 << " " << q2 << " " << q1 << " " << q0 << std::endl;

	SE3 = transpose(mat4x4(       2 * (std::pow(q0, 2) + std::pow(q1, 2)) - 1, 2 * (q1 * q2 - q0 * q3), 2 * (q1 * q3 + q0 * q2), 0,
									 2 * (q1 * q2 + q0 * q3), 2 * (std::pow(q0, 2) + std::pow(q2, 2)) - 1, 2 * (q2 * q3 - q0 * q1), 0,
									 2 * (q1 * q3 - q0 * q2), 2 * (q2 *q3 + q0 *q1), 2 * (std::pow(q0, 2) + std::pow(q3, 2)) - 1, 0,
									 0, 0, 0, 1));
}

void Rendering::writeRotation(){
	mQueue.writeBuffer(mUniformBuffer, mUniformStride + offsetof(MyUniforms, rotation), &SE3, sizeof(MyUniforms::rotation));
}

void Rendering::initTexture(){
	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = mDepthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = {WINDOW_WIDTH, WINDOW_HEIGHT, 1};
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

void Rendering::loadGeometry(const std::string& url, int uniformID){//"/Globe.obj"
	if(uniformID > MAX_NUM_UNIFORMS){
		std::cerr << "Could not load Mesh! UniformID Exceedes Buffer Size" << std::endl;
		throw std::runtime_error("Could not load Mesh! Mesh Count Has Exceeded Buffer Size");
	}

	mVertexDatas.push_back(std::vector<VertexAttributes>());
	mUniformIndices.push_back(uniformID);

	Assimp::Importer MeshImporter;

	const aiScene* scene = aiImportFile((std::string(RESOURCE_DIR) + url).c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

	if(!scene){
		std::cerr << "Could not load scene! ASSIMP ERROR: " << aiGetErrorString() << std::endl;
		throw std::runtime_error("Could not load scene!");
	}

	//Load all of the meshes
	std::uint32_t index = 0;
	for(std::uint32_t meshIdx = 0u; meshIdx < scene->mNumMeshes; ++meshIdx){
		aiMesh* mesh = scene->mMeshes[meshIdx];

		//Grab the color of the mesh
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		aiColor4D color;
		aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color);

		std::cout << "color: " << color.r << "," << color.b << ", " << color.g << ", " << color.a << std::endl;
		for(std::uint32_t faceIdx = 0u; faceIdx < mesh->mNumFaces; ++faceIdx){
			//Put the vertices in the format the webgpu/rendering pipelin is looking for
			for(int i = 0; i < 3; ++i){
				std::uint32_t vertIdx = mesh->mFaces[faceIdx].mIndices[i];
				aiVector3D vertex = mesh->mVertices[vertIdx];
				aiVector3D normal = mesh->mNormals[vertIdx];

				mVertexDatas[mVertexDatas.size() - 1].push_back(VertexAttributes());

				mVertexDatas[mVertexDatas.size() - 1][index].position = {
					vertex.x,
					-vertex.z,
					vertex.y
				};

				mVertexDatas[mVertexDatas.size() - 1][index].normal = {
					normal.x,
					-normal.z,
					normal.y
				};

				mVertexDatas[mVertexDatas.size() - 1][index].color = {
					color.r,
					color.g,
					color.b
				};

				index++;
			}			
		}
	}

	aiReleaseImport(scene);

	if(mVertexDatas[mVertexDatas.size() - 1].size() * sizeof(decltype(mVertexDatas[mVertexDatas.size() - 1][0])) > MAX_BUFFER_SIZE){
		std::cerr 	<< "Could not load geometry! Mesh Of Size: " << mVertexDatas[mVertexDatas.size() - 1].size() * sizeof(decltype(mVertexDatas[mVertexDatas.size() - 1][0])) 
					<< " Is Too Large For Buffer Of Size " << MAX_BUFFER_SIZE << std::endl;
		throw std::runtime_error("Could not load geometry! Mesh Too Large");
	}

	initVertexBuffer();
}

void Rendering::initVertexBuffer(){
	// Create vertex buffer
	BufferDescriptor bufferDesc;
	bufferDesc.size = mVertexDatas[mVertexDatas.size() - 1].size() * sizeof(VertexAttributes); // changed
	if(mVertexDatas[mVertexDatas.size() - 1].size() * sizeof(decltype(mVertexDatas[mVertexDatas.size() - 1][0])) > MAX_BUFFER_SIZE){
		std::cerr 	<< "Could not load geometry! Mesh Of Size: " << mVertexDatas[mVertexDatas.size() - 1].size() * sizeof(decltype(mVertexDatas[mVertexDatas.size() - 1][0])) 
					<< " Is Too Large For Buffer Of Size " << MAX_BUFFER_SIZE << std::endl;
		throw std::runtime_error("Could not load geometry! Mesh Too Large");
	}
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	mVertexBuffers.push_back(mDevice.createBuffer(bufferDesc));
	mQueue.writeBuffer(mVertexBuffers[mVertexBuffers.size() - 1], 0, mVertexDatas[mVertexDatas.size() - 1].data(), bufferDesc.size); // changed
 
	mIndexCounts.push_back(static_cast<int>(mVertexDatas[mVertexDatas.size() - 1].size())); // changed

	for(const auto& v : mVertexDatas){
		for(const auto& vertex: v){
			std::cout << "color: " << vertex.color.r << ", " << vertex.color.g << ", " << vertex.color.b << std::endl; 
		}
	}
}

void Rendering::initUniformBuffer(){
	BufferDescriptor bufferDesc;
	bufferDesc.size = MAX_BUFFER_SIZE; // changed
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	// Create uniform buffer
	mUniformStride = std::ceil(static_cast<double>(sizeof(MyUniforms))/mSupportedLimits.limits.minUniformBufferOffsetAlignment) * mSupportedLimits.limits.minUniformBufferOffsetAlignment;
	bufferDesc.size = (MAX_NUM_UNIFORMS - 1) * mUniformStride + sizeof(MyUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	mUniformBuffer = mDevice.createBuffer(bufferDesc);
}

void Rendering::initUniforms(int index, const mat4x4& rotation){
	// Build transform matrices

	// Translate the view
	vec3 focalPoint(0.0, 0.0, -2.0);
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

	float ratio = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
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
	mUniforms.rotation = rotation;
	mQueue.writeBuffer(mUniformBuffer, index * mUniformStride, &mUniforms, sizeof(MyUniforms));
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
	return GLFW::shouldWindowClose(mWindow);
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
	depthStencilAttachment.stencilLoadOp = LoadOp::Clear;
	depthStencilAttachment.stencilStoreOp = StoreOp::Store;
	depthStencilAttachment.stencilReadOnly = true;

	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

	renderPassDesc.timestampWriteCount = 0;
	renderPassDesc.timestampWrites = nullptr;
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

	renderPass.setPipeline(mPipeline);

	for(size_t i = 0; i < mVertexDatas.size(); ++i){
		uint32_t dynamicOffset = mUniformStride * mUniformIndices[i];

		renderPass.setVertexBuffer(0, mVertexBuffers[i], 0, mVertexDatas[i].size() * sizeof(VertexAttributes)); // changed

		// Set binding group
		renderPass.setBindGroup(0, mBindGroup, 1, &dynamicOffset);

		renderPass.draw(mIndexCounts[i], 1, 0, 0); // changed
	}
	

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

	loadGeometry("/Globe.obj", 0);

	loadGeometry("/Coords.obj", 1);

	initUniformBuffer();
	
	initUniforms(0, transpose(mat4x4(	 1, 0, 0, 0,
															0,1, 0, 0,
															0, 0, 1, 0,
															0, 0, 0, 1)));

	initUniforms(1, transpose(mat4x4(	 1, 0, 0, 0,
															0, 1, 0, 0,
															0, 0, 1, 0,
															0, 0, 0, 1)));
	
	initBinding();

	initBindGroup();
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
	for(auto& buffer : mVertexBuffers){
		buffer.destroy();
		buffer.release();
	}
	
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
}
