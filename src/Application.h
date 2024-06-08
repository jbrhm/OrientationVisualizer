#pragma once

#include <filesystem>
#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp> // all types inspired from GLSL
#include <glm/ext.hpp>
#include <fstream>
#include <assimp-3.3.1/include/assimp/Importer.hpp>
#include <assimp-3.3.1/include/assimp/cimport.h>
#include <assimp-3.3.1/include/assimp/postprocess.h>
#include <assimp-3.3.1/include/assimp/scene.h>
#include <assimp-3.3.1/include/assimp/material.h>

// Forward declare
struct GLFWwindow;

class Application {
public:


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

	// Mouse events
	void onMouseMove(double xpos, double ypos);
	void onMouseButton(int button, int action, int mods);
	void onScroll(double xoffset, double yoffset);

private:
	bool initWindowAndDevice();
	void terminateWindowAndDevice();

	bool initSwapChain();
	void terminateSwapChain();

	bool initDepthBuffer();
	void terminateDepthBuffer();

	bool initRenderPipeline();
	void terminateRenderPipeline();

	bool initTexture();
	void terminateTexture();

	bool initGeometry();
	void terminateGeometry();

	void terminateUniforms();

	bool initBindGroup();
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
        float time;
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
	GLFWwindow* m_window = nullptr;
	wgpu::Instance m_instance = nullptr;
	wgpu::Surface m_surface = nullptr;
	wgpu::Device mDevice = nullptr;
	wgpu::Queue mQueue = nullptr;
	// Keep the error callback alive
	std::unique_ptr<wgpu::ErrorCallback> m_errorCallbackHandle;

	// Swap Chain
	wgpu::SwapChain m_swapChain = nullptr;

	// Depth Buffer
	wgpu::Texture m_depthTexture = nullptr;
	wgpu::TextureView m_depthTextureView = nullptr;

	// Render Pipeline
	wgpu::ShaderModule m_shaderModule = nullptr;
	wgpu::RenderPipeline m_pipeline = nullptr;

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
	MyUniforms mUniforms;

	// Uniform Variables
	float angle1;
	float angle2;
    glm::mat4x4 S;
	glm::mat4x4 T1;
	glm::mat4x4 R1;


	// CONSTANTS
	wgpu::Buffer mUniformBuffer = nullptr;
	size_t mUniformStride;
	wgpu::SupportedLimits mSupportedLimits;


	static constexpr int WINDOW_WIDTH = 1280;
	static constexpr int WINDOW_HEIGHT = 720;

	static constexpr int MAX_NUM_UNIFORMS = 2;

	static constexpr int MAX_BUFFER_SIZE = 1000000 * sizeof(VertexAttributes);

	static constexpr int bindGroupLayoutDescEntryCount = 1;
};
