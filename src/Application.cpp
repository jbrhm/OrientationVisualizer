#include "Application.h"
#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include <iostream>
#include <cassert>
#include <filesystem>
#include <sstream>
#include <string>
#include <array>

using namespace wgpu;
using glm::mat4x4;
using glm::vec4;
using glm::vec3;

constexpr float PI = 3.14159265358979323846f;

///////////////////////////////////////////////////////////////////////////////
// Public methods

bool Application::onInit() {
	if (!initWindowAndDevice()) return false;
	if (!initSwapChain()) return false;
	if (!initDepthBuffer()) return false;
	if (!initRenderPipeline()) return false;

	loadGeometry("Globe.obj", 0);

	loadGeometry("coords.obj", 1);

	initUniformBuffer();
	
	initUniforms(0, transpose(mat4x4(	 1, 0, 0, 0,
															0,1, 0, 0,
															0, 0, 1, 0,
															0, 0, 0, 1)));
	
	initUniforms(1, transpose(mat4x4(	 1, 0, 0, 0,
															0, 1, 0, 0,
															0, 0, 1, 0,
															0, 0, 0, 1)));
	if (!initBindGroup()) return false;
	if (!initGui()) return false;
	return true;
}

void Application::onFrame() {
	glfwPollEvents();

	// Update uniform buffer
	mUniforms.time = static_cast<float>(glfwGetTime()); // glfwGetTime returns a double
	// Update view matrix
	angle1 = mUniforms.time;
	R1 = glm::rotate(mat4x4(1.0), angle1, vec3(0.0, 0.0, 1.0));
	mUniforms.modelMatrix = R1 * T1 * S;
	mQueue.writeBuffer(mUniformBuffer, offsetof(MyUniforms, modelMatrix), &mUniforms.modelMatrix, sizeof(MyUniforms::modelMatrix));

	TextureView nextTexture = m_swapChain.getCurrentTextureView();
	if (!nextTexture) {
		std::cerr << "Cannot acquire next swap chain texture" << std::endl;
		return;
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
	depthStencilAttachment.view = m_depthTextureView;
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

	renderPass.setPipeline(m_pipeline);

	// Set binding group
	uint32_t dynamicOffset = 0;
	for(size_t i = 0; i < mVertexDatas.size(); ++i){
		dynamicOffset = mUniformStride * mUniformIndices[i];

		renderPass.setVertexBuffer(0, mVertexBuffers[i], 0, mVertexDatas[i].size() * sizeof(VertexAttributes)); // changed

		// Set binding group
		renderPass.setBindGroup(0, mBindGroup, 1, &dynamicOffset);

		renderPass.draw(mIndexCounts[i], 1, 0, 0); // changed
	}

	// We add the GUI drawing commands to the render pass
	updateGui(renderPass);

	writeRotation();

	std::cout << "Current Quaternion Input: " << q0 << ", " << q1 << ", " << q2 << ", " << q3 << std::endl; 

	renderPass.end();
	renderPass.release();
	
	nextTexture.release();

	CommandBufferDescriptor cmdBufferDescriptor{};
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.finish(cmdBufferDescriptor);
	encoder.release();
	mQueue.submit(command);
	command.release();

	m_swapChain.present();

#ifdef WEBGPU_BACKEND_DAWN
	// Check for pending error callbacks
	mDevice.tick();
#endif
}

void Application::onFinish() {
	terminateGui();
	terminateBindGroup();
	terminateUniforms();
	terminateGeometry();
	terminateRenderPipeline();
	terminateDepthBuffer();
	terminateSwapChain();
	terminateWindowAndDevice();
}

bool Application::isRunning() {
	return !glfwWindowShouldClose(m_window);
}

void Application::onResize() {
	// Terminate in reverse order
	terminateDepthBuffer();
	terminateSwapChain();

	// Re-init
	initSwapChain();
	initDepthBuffer();

}

///////////////////////////////////////////////////////////////////////////////
// Private methods

bool Application::initWindowAndDevice() {
	m_instance = createInstance(InstanceDescriptor{});
	if (!m_instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return false;
	}

	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Learn WebGPU", NULL, NULL);
	if (!m_window) {
		std::cerr << "Could not open window!" << std::endl;
		return false;
	}

	std::cout << "Requesting adapter..." << std::endl;
	m_surface = glfwGetWGPUSurface(m_instance, m_window);
	RequestAdapterOptions adapterOpts{};
	adapterOpts.compatibleSurface = m_surface;
	Adapter adapter = m_instance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	adapter.getLimits(&mSupportedLimits);

	std::cout << "Requesting device..." << std::endl;
	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 4;
	requiredLimits.limits.maxVertexBuffers = 1;
	requiredLimits.limits.maxBufferSize = 150000 * sizeof(VertexAttributes);
	requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
	requiredLimits.limits.minStorageBufferOffsetAlignment = mSupportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.minUniformBufferOffsetAlignment = mSupportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.maxInterStageShaderComponents = 8;
	requiredLimits.limits.maxBindGroups = 2;
	//                                    ^ This was a 1
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	requiredLimits.limits.maxUniformBufferBindingSize = sizeof(MyUniforms);
	// Allow textures up to 2K
	requiredLimits.limits.maxTextureDimension1D = 2048;
	requiredLimits.limits.maxTextureDimension2D = 2048;
	requiredLimits.limits.maxTextureArrayLayers = 1;
	requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
	requiredLimits.limits.maxSamplersPerShaderStage = 1;
	requiredLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;

	DeviceDescriptor deviceDesc;
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.label = "The default queue";
	mDevice = adapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << mDevice << std::endl;

	// Add an error callback for more debug info
	m_errorCallbackHandle = mDevice.setUncapturedErrorCallback([](ErrorType type, char const* message) {
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl;
	});

	mQueue = mDevice.getQueue();
	adapter.release();
	return mDevice != nullptr;
}

void Application::terminateWindowAndDevice() {
	mQueue.release();
	mDevice.release();
	m_surface.release();
	m_instance.release();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}


bool Application::initSwapChain() {
	// Get the current size of the window's framebuffer:
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	std::cout << "Creating swapchain..." << std::endl;
	SwapChainDescriptor swapChainDesc;
	swapChainDesc.width = static_cast<uint32_t>(width);
	swapChainDesc.height = static_cast<uint32_t>(height);
	swapChainDesc.usage = TextureUsage::RenderAttachment;
	swapChainDesc.format = mSwapChainFormat;
	swapChainDesc.presentMode = PresentMode::Fifo;
	m_swapChain = mDevice.createSwapChain(m_surface, swapChainDesc);
	std::cout << "Swapchain: " << m_swapChain << std::endl;
	return m_swapChain != nullptr;
}

void Application::terminateSwapChain() {
	m_swapChain.release();
}


bool Application::initDepthBuffer() {
	// Get the current size of the window's framebuffer:
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = mDepthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat*)&mDepthTextureFormat;
	m_depthTexture = mDevice.createTexture(depthTextureDesc);
	std::cout << "Depth texture: " << m_depthTexture << std::endl;

	// Create the view of the depth texture manipulated by the rasterizer
	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::_2D;
	depthTextureViewDesc.format = mDepthTextureFormat;
	m_depthTextureView = m_depthTexture.createView(depthTextureViewDesc);
	std::cout << "Depth texture view: " << m_depthTextureView << std::endl;

	return m_depthTextureView != nullptr;
}

void Application::terminateDepthBuffer() {
	m_depthTextureView.release();
	m_depthTexture.destroy();
	m_depthTexture.release();
}

void Application::writeRotation(){
	SE3 = transpose(mat4x4(       2 * (std::pow(q0, 2) + std::pow(q1, 2)) - 1, 2 * (q1 * q2 - q0 * q3), 2 * (q1 * q3 + q0 * q2), 0,
									 2 * (q1 * q2 + q0 * q3), 2 * (std::pow(q0, 2) + std::pow(q2, 2)) - 1, 2 * (q2 * q3 - q0 * q1), 0,
									 2 * (q1 * q3 - q0 * q2), 2 * (q2 *q3 + q0 *q1), 2 * (std::pow(q0, 2) + std::pow(q3, 2)) - 1, 0,
									 0, 0, 0, 1));
	mQueue.writeBuffer(mUniformBuffer, mUniformStride + offsetof(MyUniforms, rotation), &SE3, sizeof(MyUniforms::rotation));
}

ShaderModule Application::loadShaderModule(const std::filesystem::path& path, Device device) {
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


bool Application::initRenderPipeline() {
	std::cout << "Creating shader module..." << std::endl;
	m_shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", mDevice);
	std::cout << "Shader module: " << m_shaderModule << std::endl;

	std::cout << "Creating render pipeline..." << std::endl;
	RenderPipelineDescriptor pipelineDesc;

	// Vertex fetch
	std::vector<VertexAttribute> vertexAttribs(3);

	// Position attribute
	vertexAttribs[0].shaderLocation = 0;
	vertexAttribs[0].format = VertexFormat::Float32x3;
	vertexAttribs[0].offset = 0;

	// Normal attribute
	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = VertexFormat::Float32x3;
	vertexAttribs[1].offset = offsetof(VertexAttributes, normal);

	// Color attribute
	vertexAttribs[2].shaderLocation = 2;
	vertexAttribs[2].format = VertexFormat::Float32x3;
	vertexAttribs[2].offset = offsetof(VertexAttributes, color);

	VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
	vertexBufferLayout.attributes = vertexAttribs.data();
	vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;

	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

	pipelineDesc.vertex.module = m_shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDesc.primitive.frontFace = FrontFace::CCW;
	pipelineDesc.primitive.cullMode = CullMode::None;

	FragmentState fragmentState;
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = m_shaderModule;
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
	colorTarget.format = mSwapChainFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All;

	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;

	DepthStencilState depthStencilState = Default;
	depthStencilState.depthCompare = CompareFunction::Less;
	depthStencilState.depthWriteEnabled = true;
	depthStencilState.format = mDepthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

	pipelineDesc.depthStencil = &depthStencilState;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Create binding layouts
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
	PipelineLayout layout = mDevice.createPipelineLayout(layoutDesc);
	pipelineDesc.layout = layout;

	m_pipeline = mDevice.createRenderPipeline(pipelineDesc);
	std::cout << "Render pipeline: " << m_pipeline << std::endl;

	return m_pipeline != nullptr;
}

void Application::terminateRenderPipeline() {
	m_pipeline.release();
	m_shaderModule.release();
	mBindGroupLayout.release();
}

void Application::loadGeometry(const std::string& url, int uniformID){//"/Globe.obj"
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

void Application::initVertexBuffer(){
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
}

void Application::terminateGeometry() {
	for(auto buff : mVertexBuffers){
		buff.destroy();
		buff.release();
	}
}

void Application::terminateUniforms() {
	mUniformBuffer.destroy();
	mUniformBuffer.release();
}


bool Application::initBindGroup() {
	// Create a binding
	std::vector<BindGroupEntry> bindings(1);

	bindings[0].binding = 0;
	bindings[0].buffer = mUniformBuffer;
	bindings[0].offset = 0;
	bindings[0].size = sizeof(MyUniforms);

	BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = mBindGroupLayout;
	bindGroupDesc.entryCount = (uint32_t)bindings.size();
	bindGroupDesc.entries = bindings.data();
	mBindGroup = mDevice.createBindGroup(bindGroupDesc);

	return mBindGroup != nullptr;
}

void Application::terminateBindGroup() {
	mBindGroup.release();
}

void Application::initUniforms(int index, const mat4x4& rotation){
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

void Application::initUniformBuffer(){
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

bool Application::initGui() {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOther(m_window, true);
	ImGui_ImplWGPU_InitInfo initInfo;
	initInfo.DepthStencilFormat = mDepthTextureFormat;
	initInfo.RenderTargetFormat = mSwapChainFormat;
	initInfo.Device = mDevice;
	initInfo.NumFramesInFlight = 3;
	ImGui_ImplWGPU_Init(&initInfo);
	return true;
}

void Application::terminateGui() {
	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplWGPU_Shutdown();
}

void Application::updateGui(RenderPassEncoder renderPass) {
	// Start the Dear ImGui frame
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Build our UI
	{
		f = 0.0f;
		static int counter = 0;
		static bool show_demo_window = true;
		static bool show_another_window = false;
		static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		ImGui::Begin("Hello, world!");                                // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");                     // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);            // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);                  // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color);       // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                                  // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::SetNextItemWidth(100);
		ImGui::InputScalar("q0", 9, &q0); // Input type 9 is double
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputScalar("q1", 9, &q1); // Input type 9 is double
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputScalar("q2", 9, &q2); // Input type 9 is double
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::InputScalar("q3", 9, &q3); // Input type 9 is double
		ImGui::SameLine();
		ImGui::End();
	}

	// Draw the UI
	ImGui::EndFrame();
	// Convert the UI defined above into low-level drawing commands
	ImGui::Render();
	// Execute the low-level drawing commands on the WebGPU backend
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}
