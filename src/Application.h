#pragma once

// STL
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cassert>
#include <sstream>
#include <string>
#include <array>
#include <math.h>

// WEBGPU
#include <webgpu/webgpu.hpp>

// GLFW3WEBGPU
#include <glfw3webgpu.h>

// GLM
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>

// ASSIMP
#include <assimp-3.3.1/include/assimp/Importer.hpp>
#include <assimp-3.3.1/include/assimp/cimport.h>
#include <assimp-3.3.1/include/assimp/postprocess.h>
#include <assimp-3.3.1/include/assimp/scene.h>
#include <assimp-3.3.1/include/assimp/material.h>

// IMGUI
#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

// EIGEN
#include <Eigen/Core>
#include <Eigen/QR>

// Codebase
#include "LieAlgebra.hpp"
#include "utils.hpp"
#include "GLFW.hpp"

#ifdef DEBUG 
	constexpr bool isDebug = true;
#else
	constexpr bool isDebug = false;
#endif

struct GLFWwindow;

class Application {
private:
	struct Uniform {
		// View Adjustment Matrices
        glm::mat4x4 projectionMatrix;
        glm::mat4x4 viewMatrix;
        glm::mat4x4 modelMatrix;
        glm::mat4x4 rotation;
        // Color
        std::array<float, 4> color;
		// How far the mesh should scale in z direction (vector)
        float zScalar;
        float _pad[3];
    };

	// check byte alignment
	static_assert(sizeof(Uniform) % 16 == 0);

	// Fields each vertex will have
	struct VertexAttributes {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
    };  

	// Window
	GLFW::WindowPtr mWindow = nullptr;

	// Instance
	wgpu::Instance mInstance = nullptr;

	// Device
	wgpu::SupportedLimits mSupportedLimits;
	wgpu::Device mDevice = nullptr;

	// Device Error Callback
	std::unique_ptr<wgpu::ErrorCallback> mErrorCallback;

	// Surface
	wgpu::Surface mSurface = nullptr;

	// Queue
	wgpu::Queue mQueue = nullptr;

	// Swap Chain
	wgpu::SwapChain mSwapChain = nullptr;
	wgpu::TextureFormat mSwapChainFormat = wgpu::TextureFormat::BGRA8Unorm;

	// Textures
	wgpu::Texture mDepthTexture = nullptr;
	wgpu::TextureView mDepthTextureView = nullptr;
	wgpu::TextureFormat mDepthTextureFormat = wgpu::TextureFormat::Depth24Plus;

	// Render Pipeline
	wgpu::ShaderModule mShaderModule = nullptr;
	wgpu::RenderPipeline mRenderPipeline = nullptr;
	
	// Bindings
	wgpu::BindGroup mBindGroup = nullptr;
	wgpu::BindGroupLayoutEntry mBindingLayout;
	wgpu::BindGroupLayout mBindGroupLayout = nullptr;
	static constexpr int bindGroupLayoutDescEntryCount = 1;

	// Mesh Data
	std::vector<std::vector<VertexAttributes>> mVertexDatas;
	std::vector<int> mIndexCounts;

	// Buffer Data
	std::vector<wgpu::Buffer> mVertexBuffers;
	std::vector<int> mUniformIndices;

	// Uniforms
	wgpu::Buffer mUniformBuffer = nullptr;
	Uniform mUniforms;
	glm::mat4x4 SE3;
	float mZScalar = 1.0f;

	// Uniform Vars
	float angle1;
	float angle2;
    glm::mat4x4 S;
	glm::mat4x4 T1;
	glm::mat4x4 R1;

	//IMGUI variables
	float imguiScale = 2.0f;
	int inputBoxSize = 76;

	// Quaternion
	bool isQuaternion = true;
	double q0 = 1.0;
	double q1 = 0.0;
	double q2 = 0.0;
	double q3 = 0.0;

	// SO3 Matrix
	bool isSO3 = false;
	double i00 = 1, i01 = 0, i02 = 0;
	double i10 = 0, i11 = 1, i12 = 0;
	double i20 = 0, i21 = 0, i22 = 1;

	// Lie minus operation
	bool isLieAlgebra = false;
	
	// Lie Algebra Operation
	bool isAdd = false;
	bool isSub = true;

	// Left hand side SO3 matrix
	double l100 = 1, l101 = 0, l102 = 0;
	double l110 = 0, l111 = 1, l112 = 0;
	double l120 = 0, l121 = 0, l122 = 1;

	// Right hand side SO3 matrix
	double r100 = 1, r101 = 0, r102 = 0;
	double r110 = 0, r111 = 1, r112 = 0;
	double r120 = 0, r121 = 0, r122 = 1;
	glm::mat4x4 rotationGLM;
	
	// CONSTANTS
	size_t mUniformStride;

	// Window sizing
	static constexpr int WINDOW_WIDTH = 1920;
	static constexpr int WINDOW_HEIGHT = 1080;

	// Imgui constants
	static constexpr int IMGUI_DOUBLE_SCALAR = 9;
	static constexpr int IMGUI_FLOAT_SCALAR = 8;

	// Maximum number of uniforms for the meshes
	static constexpr int MAX_NUM_UNIFORMS = 3;

	// Max mesh buffer size
	static constexpr int MAX_BUFFER_SIZE = 1000000 * sizeof(VertexAttributes);

	void initGLFW();
	void terminateGLFW();

	void initAdapterAndDevice();
	void terminateAapterAndDevice();

	void initQueue();
	void terminateQueue();

	void initSwapChain();
	void terminateSwapChain();

	void initDepthBuffer();
	void terminateDepthBuffer();

	void initRenderPipeline();
	void terminateRenderPipeline();

	wgpu::ShaderModule loadShaderModule(const std::filesystem::path& path, wgpu::Device device);

	void loadGeometry(const std::string& url, int uniformID);
	void initVertexBuffer();
	void terminateGeometry();

	void initUniformBuffer();
	void initUniform(int index, const glm::mat4x4& rotation, float x, float y, float z);
	void terminateUniforms();

	void initBindGroup();
	void terminateBindGroup();

	void initGUI();
	void terminateGUI();
	void updateGUI(wgpu::RenderPassEncoder renderPass);

	void writeRotation();

	void adjustView(float x, float y, float z);

public:
	Application(); 

	~Application();

	bool init();

	void updateFrame();

	void terminate();

	bool isOpen();
};
