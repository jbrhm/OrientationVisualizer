#pragma once

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>


#include <glm/glm.hpp> // all types inspired from GLSL
#include <glm/ext.hpp>

#include <webgpu/webgpu.hpp>

#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <array>
#include <mutex>

constexpr float PI = 3.14159265358979323846f;
constexpr int MAX_NUM_UNIFORMS = 2;

// Window Dimensions
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

class Rendering{
private:
    struct VertexAttributes {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
    };

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

    // WebGPU Objects
    wgpu::Instance mInstance = nullptr;
    wgpu::Adapter mAdapter = nullptr;
    wgpu::Surface mSurface = nullptr;
    wgpu::SupportedLimits mSupportedLimits;
    wgpu::Device mDevice = nullptr;
    wgpu::Queue mQueue = nullptr;
    wgpu::SwapChain mSwapChain = nullptr;
    wgpu::ShaderModule mShaderModule = nullptr;
    wgpu::Texture mDepthTexture = nullptr;
    wgpu::TextureView mDepthTextureView = nullptr;
    std::vector<wgpu::Buffer> mVertexBuffers;
    wgpu::Buffer mUniformBuffer = nullptr;
    wgpu::BindGroupEntry mBinding;
    wgpu::BindGroup mBindGroup = nullptr;

    //Render Pipelin Objects
    std::vector<wgpu::VertexAttribute> mVertexAttribs;
    wgpu::VertexBufferLayout mVertexBufferLayout;
    wgpu::FragmentState mFragmentState;
    wgpu::BlendState mBlendState;
    wgpu::ColorTargetState mColorTarget;
    wgpu::DepthStencilState mDepthStencilState;
    wgpu::BindGroupLayoutEntry mBindingLayout;
    wgpu::BindGroupLayout mBindGroupLayout = nullptr;
    wgpu::PipelineLayout mPipelinelayout = nullptr;
    wgpu::RenderPipeline mPipeline = nullptr;

    // Mesh Data
	std::vector<std::vector<VertexAttributes>> mVertexDatas;
    std::vector<int> mUniformIndices;
    std::vector<int> mIndexCounts;

    // Uniform Objects
    MyUniforms mUniforms;
    float angle1;
    float angle2;
    glm::mat4x4 S;
    glm::mat4x4 T1;
    glm::mat4x4 R1;
    unsigned int mUniformStride;
    glm::mat4x4 SE3;

    // Const values
    wgpu::TextureFormat mSwapChainFormat = wgpu::TextureFormat::BGRA8Unorm;

    wgpu::TextureFormat mDepthTextureFormat = wgpu::TextureFormat::Depth24Plus;

    static constexpr int bindGroupLayoutDescEntryCount = 1;

    static constexpr int MAX_BUFFER_SIZE = 1000000 * sizeof(VertexAttributes);

    // GLFW Objects
    GLFWwindow* mWindow = nullptr;

    void initInstance();

    void initGLFW();

    void initAdapter();

    void initDevice();

    void initQueue();

    void initSwapChain();

    void initShaderModule();

    void initRenderPipeline();

    void initTexture();

    void initTextureView();

    void loadGeometry(const std::string& url, int uniformID);

    void initVertexBuffer();

    void initUniformBuffer();

    void initUniforms(int index, const glm::mat4x4& rotation);

    void initBinding();

    void initBindGroup();

    wgpu::ShaderModule loadShaderModule(const std::filesystem::path& path, wgpu::Device device);

public:
    Rendering();

    bool shouldWindowClose();

    void render();

    void getNewInput();

    void writeRotation();

    ~Rendering();
};