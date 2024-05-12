#pragma once 

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>


#include <glm/glm.hpp> // all types inspired from GLSL
#include <glm/ext.hpp>

#include <webgpu/webgpu.hpp>

#include "tiny_obj_loader/tiny_obj_loader.h"

#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <array>
#include <memory>
#include <exception>

constexpr float PI = 3.14159265358979323846f;

class Rendering {
private:

    //VertexAttributes Struct
    struct VertexAttributes {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
    };

    struct MyUniforms {
        glm::mat4x4 projectionMatrix;
        glm::mat4x4 viewMatrix;
        glm::mat4x4 modelMatrix;
        std::array<float, 4> color;
        float time;
        float _pad[3];
    };

    //Pointers
	std::unique_ptr<wgpu::Instance> mInstance;
    std::unique_ptr<GLFWwindow*> mWindow;
    std::unique_ptr<wgpu::Surface> mSurface;
    std::unique_ptr<wgpu::Adapter> mAdapter;
    std::unique_ptr<wgpu::SupportedLimits> mAdapterLimits;
    std::unique_ptr<wgpu::Device> mDevice;
    std::unique_ptr<wgpu::Queue> mQueue;
    std::unique_ptr<wgpu::SwapChain> mSwapChain;
    std::unique_ptr<wgpu::ShaderModule> mShaderModule;
    std::unique_ptr<wgpu::PipelineLayout> mLayout;
    std::unique_ptr<wgpu::RenderPipeline> mRenderPipeline;
    std::unique_ptr<wgpu::Texture> mTexture;
    std::unique_ptr<wgpu::TextureView> mTextureView;
    MyUniforms mUniforms;
    std::unique_ptr<wgpu::BindGroupLayout> mBindGroupLayout;
    std::unique_ptr<wgpu::BindGroup> mBindGroup;

    wgpu::TextureFormat swapChainFormat = wgpu::TextureFormat::BGRA8Unorm;

    wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;

    // Mesh Data
    std::vector<float> pointData;
	std::vector<uint16_t> indexData;
    std::vector<VertexAttributes> vertexData;
    
    //Vertex Buffer Data
    std::unique_ptr<wgpu::Buffer> mVertexBuffer;
    std::unique_ptr<wgpu::Buffer> mUniformBuffer;
    int indexCount;

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;

    std::vector<wgpu::VertexAttribute> vertexAttribs;


    //Matrix Constants
    float angle1; // arbitrary time
    float angle2;
    glm::mat4x4 S;
    glm::mat4x4 T1;
    glm::mat4x4 R1;



    void initInstance();

    void initGLFW();

    void initAdapter();

    void initDevice();

    void initQueue();

    void initSwapChain();

    void initShaderModule();

    wgpu::ShaderModule loadShaderModule(const std::filesystem::path& path, wgpu::Device device);

    void initPipeline();

    void initTexture();

    void initTextureView();

    bool loadGeometryFromObj(const std::filesystem::path& path, std::vector<VertexAttributes>& vertexData);

    void writeVertexDataToGPU();

    void initUniforms();

    void initBindGroup();

public:
    Rendering(/* args */);

    void loadGeometry();

    void renderTexture();

    ~Rendering();
};
