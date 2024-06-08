#pragma once

#include <webgpu/webgpu.hpp>
#include <glm/glm.hpp>
#include <filesystem>
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

	bool initUniforms();
	void terminateUniforms();

	bool initBindGroup();
	void terminateBindGroup();

	void updateProjectionMatrix();
	void updateViewMatrix();
	void updateDragInertia();

	bool initGui(); // called in onInit
	void terminateGui(); // called in onFinish
	void updateGui(wgpu::RenderPassEncoder renderPass); // called in onFrame

    wgpu::ShaderModule loadShaderModule(const std::filesystem::path& path, wgpu::Device device);

	void loadGeometry(const std::string& url, int uniformID);

private:
	// (Just aliases to make notations lighter)
	using mat4x4 = glm::mat4x4;
	using vec4 = glm::vec4;
	using vec3 = glm::vec3;
	using vec2 = glm::vec2;

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

	struct CameraState {
		// angles.x is the rotation of the camera around the global vertical axis, affected by mouse.x
		// angles.y is the rotation of the camera around its local horizontal axis, affected by mouse.y
		vec2 angles = { 0.8f, 0.5f };
		// zoom is the position of the camera along its local forward axis, affected by the scroll wheel
		float zoom = -1.2f;
	};

	struct VertexAttributes {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
    };  

	// Window and Device
	GLFWwindow* m_window = nullptr;
	wgpu::Instance m_instance = nullptr;
	wgpu::Surface m_surface = nullptr;
	wgpu::Device m_device = nullptr;
	wgpu::Queue m_queue = nullptr;
	// Keep the error callback alive
	std::unique_ptr<wgpu::ErrorCallback> m_errorCallbackHandle;

	// Swap Chain
	wgpu::SwapChain m_swapChain = nullptr;

	// Depth Buffer
	wgpu::Texture m_depthTexture = nullptr;
	wgpu::TextureView m_depthTextureView = nullptr;

	// Render Pipeline
	wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
	wgpu::ShaderModule m_shaderModule = nullptr;
	wgpu::RenderPipeline m_pipeline = nullptr;

	// Texture
	wgpu::Sampler m_sampler = nullptr;
	wgpu::Texture m_texture = nullptr;
	wgpu::TextureView m_textureView = nullptr;
	
	// Geometry
	wgpu::Buffer m_vertexBuffer = nullptr;
	int m_vertexCount = 0;

	// Uniforms
	wgpu::Buffer m_uniformBuffer = nullptr;
	MyUniforms m_uniforms;

	// Bind Group
	wgpu::BindGroup m_bindGroup = nullptr;

	CameraState m_cameraState;

	// New Stuff
	std::vector<std::vector<VertexAttributes>> mVertexDatas;
	wgpu::TextureFormat mDepthTextureFormat = wgpu::TextureFormat::Depth24Plus;
	wgpu::TextureFormat mSwapChainFormat = wgpu::TextureFormat::BGRA8Unorm;
	std::vector<int> mUniformIndices;
	wgpu::BindGroupLayoutEntry mBindingLayout;
	wgpu::BindGroupLayout mBindGroupLayout = nullptr;


	// CONSTANTS
	static constexpr int MAX_NUM_UNIFORMS = 2;

	static constexpr int MAX_BUFFER_SIZE = 1000000 * sizeof(VertexAttributes);

	static constexpr int bindGroupLayoutDescEntryCount = 1;
};
