#pragma once

// STL
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cassert>
#include <sstream>
#include <string>
#include <array>

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

// Forward declare
struct GLFWwindow;

class Application {
public:
	Application(); 

	~Application();

	// A function called only once at the beginning. Returns false is init failed.
	bool onInit();

	// A function called at each frame, guaranteed never to be called before `onInit`.
	void onFrame();

	// A function called only once at the very end.
	void onFinish();

	// A function that tells if the application is still running.
	bool isRunning();

	// A function called when the window is resized.
	void onResize();

private:
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

	bool initTexture();
	void terminateTexture();

	void terminateGeometry();

	void terminateUniforms();

	void initBindGroup();
	void terminateBindGroup();

	void updateViewMatrix();
	void updateDragInertia();

	bool initGui(); // called in onInit
	void terminateGui(); // called in onFinish
	void updateGui(wgpu::RenderPassEncoder renderPass); // called in onFrame

    wgpu::ShaderModule loadShaderModule(const std::filesystem::path& path, wgpu::Device device);

	void loadGeometry(const std::string& url, int uniformID);

private:

	/**
	 * The same structure as in the shader, replicated in C++
	 */
	struct MyUniforms {
        // We add transform matrices
        glm::mat4x4 projectionMatrix;
        glm::mat4x4 viewMatrix;
        glm::mat4x4 modelMatrix;
        glm::mat4x4 rotation;
        std::array<float, 4> color;
        float zScalar;
        float _pad[3];
    };
	// Have the compiler check byte alignment
	static_assert(sizeof(MyUniforms) % 16 == 0);

	struct VertexAttributes {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
    };  

	// Window and Device
	GLFW::WindowPtr mWindow = nullptr;
	wgpu::Instance mInstance = nullptr;
	wgpu::Surface mSurface = nullptr;
	wgpu::Device mDevice = nullptr;
	wgpu::Queue mQueue = nullptr;
	// Keep the error callback alive
	std::unique_ptr<wgpu::ErrorCallback> mErrorCallback;

	// Swap Chain
	wgpu::SwapChain mSwapChain = nullptr;

	// Depth Buffer
	wgpu::Texture mDepthTexture = nullptr;
	wgpu::TextureView mDepthTextureView = nullptr;

	// Render Pipeline
	wgpu::ShaderModule mShaderModule = nullptr;
	wgpu::RenderPipeline mRenderPipeline = nullptr;

	// Texture
	wgpu::Sampler m_sampler = nullptr;
	wgpu::Texture m_texture = nullptr;
	wgpu::TextureView m_textureView = nullptr;
	
	// Geometry
	wgpu::Buffer m_vertexBuffer = nullptr;
	int m_vertexCount = 0;
	
	// Bind Group
	wgpu::BindGroup mBindGroup = nullptr;


	// New Stuff
	std::vector<std::vector<VertexAttributes>> mVertexDatas;
	wgpu::TextureFormat mDepthTextureFormat = wgpu::TextureFormat::Depth24Plus;
	wgpu::TextureFormat mSwapChainFormat = wgpu::TextureFormat::BGRA8Unorm;
	std::vector<int> mUniformIndices;
	wgpu::BindGroupLayoutEntry mBindingLayout;
	wgpu::BindGroupLayout mBindGroupLayout = nullptr;
	void initUniforms(int index, const glm::mat4x4& rotation);
	void initUniformBuffer();
	std::vector<wgpu::Buffer> mVertexBuffers;
	void initVertexBuffer();
	std::vector<int> mIndexCounts;
	void writeRotation();
	MyUniforms mUniforms;

	// Uniform Variables
	float angle1;
	float angle2;
    glm::mat4x4 S;
	glm::mat4x4 T1;
	glm::mat4x4 R1;

	glm::mat4x4 SE3;

	//IMGUI variables
	float imguiScale = 2.0f;
	int inputBoxSize = 76;

	bool isQuaternion = true;
	double q0 = 1.0;
	double q1 = 0.0;
	double q2 = 0.0;
	double q3 = 0.0;

	bool isSO3 = false;
	double i00 = 1, i01 = 0, i02 = 0;
	double i10 = 0, i11 = 1, i12 = 0;
	double i20 = 0, i21 = 0, i22 = 1;

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

	float mZScalar = 1.0f;
	glm::mat4x4 rotationGLM;
	

	// CONSTANTS
	wgpu::Buffer mUniformBuffer = nullptr;
	size_t mUniformStride;
	wgpu::SupportedLimits mSupportedLimits;


	static constexpr int WINDOW_WIDTH = 1920;
	static constexpr int WINDOW_HEIGHT = 1080;

	static constexpr int IMGUI_DOUBLE_SCALAR = 9;
	static constexpr int IMGUI_FLOAT_SCALAR = 8;


	static constexpr int MAX_NUM_UNIFORMS = 3;

	static constexpr int MAX_BUFFER_SIZE = 1000000 * sizeof(VertexAttributes);

	static constexpr int bindGroupLayoutDescEntryCount = 1;
};
