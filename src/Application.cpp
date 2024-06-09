#include "Application.h"
#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include <Eigen/Core>
#include <Eigen/QR>
#include "LieAlgebra.hpp"

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

int getSign(double num){
	if (num > 0) return 1;
	if (num < 0) return -1;
	return 0;
}

bool Application::onInit() {
	if (!initWindowAndDevice()) return false;
	if (!initSwapChain()) return false;
	if (!initDepthBuffer()) return false;
	if (!initRenderPipeline()) return false;

	loadGeometry("Globe.obj", 0);

	loadGeometry("coords.obj", 1);

	loadGeometry("vector.obj", 2);

	initUniformBuffer();
	
	initUniforms(0, transpose(mat4x4(	 1, 0, 0, 0,
															0,1, 0, 0,
															0, 0, 1, 0,
															0, 0, 0, 1)));
	
	initUniforms(1, transpose(mat4x4(	 0, 1, 0, 0,
															0, 0, 1, 0,
															1, 0, 0, 0,
															0, 0, 0, 1)));
	initUniforms(2, transpose(mat4x4(	 1, 0, 0, 0,
															0, 1, 0, 0,
															0, 0, 1, 0,
															0, 0, 0, 1)));
	if (!initBindGroup()) return false;
	if (!initGui()) return false;
	return true;
}

void Application::onFrame() {
	glfwPollEvents();
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

	// State Machine For Rendering different meshes depending on which mode the user has chosen
	if(isQuaternion || isSO3){
		// Update uniform buffer
		// Update view matrix
		angle1 = static_cast<float>(glfwGetTime());
		R1 = glm::rotate(mat4x4(1.0), angle1, vec3(0.0, 0.0, 1.0));
		mUniforms.modelMatrix = R1 * T1 * S;
		mQueue.writeBuffer(mUniformBuffer, offsetof(MyUniforms, modelMatrix), &mUniforms.modelMatrix, sizeof(MyUniforms::modelMatrix));
	}else if(isLieAlgebra){
		// Left hand side matrix
		Eigen::Matrix3d lhsSO3;
		lhsSO3 << 	l100, l101, l102, 
					l110, l111, l112,
					l120, l121, l122;

		// Right hand side matrix
		Eigen::Matrix3d rhsSO3;
		rhsSO3 << 	r100, r101, r102, 
					r110, r111, r112,
					r120, r121, r122;

		// Depending on the desired operation
		Eigen::Vector3d desired;
		if(isSub){
			Eigen::Matrix3d composed = lhsSO3 * rhsSO3.transpose();
			desired = LieAlgebra::logarithmicMap(composed);
		}
		
		Eigen::Vector3d v;
		v.x() = desired.x();
		v.y() = -1 * desired.y();
		v.z() = desired.z();
		
		Eigen::HouseholderQR<Eigen::Matrix3d> qr;
		Eigen::Matrix3d vectorMatrix;
		vectorMatrix << v.x(), v.x(), v.x(),
						v.y(), v.y(), v.y() + 1,
						v.z(), v.z() + 1, v.z(),
		qr.compute(vectorMatrix);

		Eigen::Matrix3d reorderMatrix = Eigen::Matrix3d::Zero();
		reorderMatrix(2, 1) = 1;
		reorderMatrix(1, 0) = 1;
		reorderMatrix(0, 2) = 1;

		Eigen::Matrix3d rotation = qr.householderQ() * reorderMatrix;

		// Make sure the signs of the vectors have not been changed by QR decomp
		if(getSign(rotation.coeff(0,2)) != getSign(v.coeff(0,0))){
			rotation(0,2) *= -1;
			rotation(1,2) *= -1;
			rotation(2,2) *= -1;
			rotation(0,1) *= -1;
			rotation(1,1) *= -1;
			rotation(2,1) *= -1;
		}

		std::cout << "Vector:\n" << desired << std::endl;

		std::cout << "Rotation:\n" << rotation << std::endl;

		mZScalar = v.norm();

		rotationGLM[0][0] = rotation.coeff(0,0);
		rotationGLM[0][1] = rotation.coeff(0,1);
		rotationGLM[0][2] = rotation.coeff(0,2);
		rotationGLM[0][3] = 0;
		rotationGLM[1][0] = rotation.coeff(1,0);
		rotationGLM[1][1] = rotation.coeff(1,1);
		rotationGLM[1][2] = rotation.coeff(1,2);
		rotationGLM[1][3] = 0;
		rotationGLM[2][0] = rotation.coeff(2,0);
		rotationGLM[2][1] = rotation.coeff(2,1);
		rotationGLM[2][2] = rotation.coeff(2,2);
		rotationGLM[2][3] = 0;
		rotationGLM[3][0] = 0;
		rotationGLM[3][1] = 0;
		rotationGLM[3][2] = 0;
		rotationGLM[3][3] = 1;

		rotationGLM = glm::transpose(rotationGLM);
		


		mQueue.writeBuffer(mUniformBuffer, 2 * mUniformStride + offsetof(MyUniforms, zScalar), &mZScalar, sizeof(MyUniforms::zScalar));

		mQueue.writeBuffer(mUniformBuffer, 2 * mUniformStride + offsetof(MyUniforms, rotation), &rotationGLM, sizeof(MyUniforms::rotation));

	}

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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
	if(isQuaternion){
		SE3 = transpose(mat4x4(   2 * (std::pow(q0, 2) + std::pow(q1, 2)) - 1, 2 * (q1 * q2 - q0 * q3), 2 * (q1 * q3 + q0 * q2), 0,
									 2 * (q1 * q2 + q0 * q3), 2 * (std::pow(q0, 2) + std::pow(q2, 2)) - 1, 2 * (q2 * q3 - q0 * q1), 0,
									 2 * (q1 * q3 - q0 * q2), 2 * (q2 *q3 + q0 *q1), 2 * (std::pow(q0, 2) + std::pow(q3, 2)) - 1, 0,
									 0, 0, 0, 1));
	}else if(isSO3){
		SE3 = transpose(mat4x4(	i00, i01, i02, 0,
									i10, i11, i12, 0,
									i20, i21, i22, 0,
									0,   0,   0,   1));
	}
	
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
					vertex.z,
					vertex.y
				};

				mVertexDatas[mVertexDatas.size() - 1][index].normal = {
					normal.x,
					normal.z,
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
	vec3 focalPoint(-0.25, 0.0, -2.0);
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

	mUniforms.zScalar = 1.0f;
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

	// Adjust font
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = imguiScale;
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
		ImGui::Begin("Orientation Visualizer");                                // Create a window called "Hello, world!" and append into it.

		ImGui::Text("Which input would you like?");                     // Display some text (you can use a format strings too)
		ImGui::Checkbox("Quaternion: ", &isQuaternion);            // Edit bools storing our window open/close state
		ImGui::Checkbox("SO3: ", &isSO3);
		ImGui::Checkbox("Lie Algebra: ", &isLieAlgebra);

		// Decision tree for each of the different display states
		if(isQuaternion){
			ImGui::Text("Ex. 0, 0, 0, 1 is the identity quaternion");
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("q0", IMGUI_DOUBLE_SCALAR, &q1); // Input type 9 is double
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("q1", IMGUI_DOUBLE_SCALAR, &q2); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("q2", IMGUI_DOUBLE_SCALAR, &q3); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("q3", IMGUI_DOUBLE_SCALAR, &q0); 
		}else if(isSO3){
			ImGui::Text("SO3 Matrix:");

			// Row 1
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("(0,0)", IMGUI_DOUBLE_SCALAR, &i00); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("(0,1)", IMGUI_DOUBLE_SCALAR, &i01); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("(0,2)", IMGUI_DOUBLE_SCALAR, &i02); 
			
			// Row 2
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("(1,0)", IMGUI_DOUBLE_SCALAR, &i10); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("(1,1)", IMGUI_DOUBLE_SCALAR, &i11); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("(1,2)", IMGUI_DOUBLE_SCALAR, &i12); 

			// Row 2
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("(2,0)", IMGUI_DOUBLE_SCALAR, &i20); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("(2,1)", IMGUI_DOUBLE_SCALAR, &i21); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("(2,2)", IMGUI_DOUBLE_SCALAR, &i22); 
		}else if(isLieAlgebra){
			ImGui::Checkbox("Add: ", &isAdd);
			ImGui::Checkbox("Subtract: ", &isSub);

			// Left Hand Side SO3 Matrix
			ImGui::Text("SO3 Matrix Left Hand Side:");
			// Row 1
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("l(0,0)", IMGUI_DOUBLE_SCALAR, &l100); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("l(0,1)", IMGUI_DOUBLE_SCALAR, &l101); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("l(0,2)", IMGUI_DOUBLE_SCALAR, &l102); 
			
			// Row 2
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("l(1,0)", IMGUI_DOUBLE_SCALAR, &l110); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("l(1,1)", IMGUI_DOUBLE_SCALAR, &l111); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("l(1,2)", IMGUI_DOUBLE_SCALAR, &l112); 

			// Row 2
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("l(2,0)", IMGUI_DOUBLE_SCALAR, &l120); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("l(2,1)", IMGUI_DOUBLE_SCALAR, &l121); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("l(2,2)", IMGUI_DOUBLE_SCALAR, &l122); 

			// RHS SO3 Matrix
			ImGui::Text("SO3 Matrix Right Hand Side:");
			// Row 1
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("r(0,0)", IMGUI_DOUBLE_SCALAR, &r100); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("r(0,1)", IMGUI_DOUBLE_SCALAR, &r101); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("r(0,2)", IMGUI_DOUBLE_SCALAR, &r102); 
			
			// Row 2
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("r(1,0)", IMGUI_DOUBLE_SCALAR, &r110); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("r(1,1)", IMGUI_DOUBLE_SCALAR, &r111); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("r(1,2)", IMGUI_DOUBLE_SCALAR, &r112); 

			// Row 2
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("r(2,0)", IMGUI_DOUBLE_SCALAR, &r120); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("r(2,1)", IMGUI_DOUBLE_SCALAR, &r121); 
			ImGui::SameLine();
			ImGui::SetNextItemWidth(inputBoxSize);
			ImGui::InputScalar("r(2,2) ", IMGUI_DOUBLE_SCALAR, &r122); 
		}
		
		// Display the refresh rate
		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
	}

	// Draw the UI
	ImGui::EndFrame();
	// Convert the UI defined above into low-level drawing commands
	ImGui::Render();
	// Execute the low-level drawing commands on the WebGPU backend
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}
