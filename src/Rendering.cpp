#include "Rendering.hpp"

using namespace wgpu;
using glm::mat4x4;
using glm::vec3;
using glm::vec4;

Rendering::Rendering() { init(); }

Rendering::~Rendering() { mInstance.release(); }

bool Rendering::init() {
  // Create WebGPU instance
  if constexpr (isDebug) {
    std::cout << "Initializing Instance..." << std::endl;
  }

  mInstance = createInstance(InstanceDescriptor{});
  if (!mInstance) {
    std::cerr << "WebGPU did not initialize properly!" << std::endl;
    throw std::runtime_error("WebGPU did not initialize properly!");
  }

  if constexpr (isDebug) {
    std::cout << "Instance: " << mInstance << std::endl;
  }

  initGLFW();

  initAdapterAndDevice();

  initQueue();

  initSwapChain();

  initDepthBuffer();

  initRenderPipeline();

  loadGeometry("Globe.obj", 0);

  loadGeometry("coords.obj", 1);

  loadGeometry("vector.obj", 2);

  initUniformBuffer();

  adjustView(-0.25, 0.0, -2.0);

  initBindGroup();

  initGUI();

  mBeginFrame = std::chrono::system_clock::now();
  mEndFrame = mBeginFrame + inverseFPS;

  return true;
}

void Rendering::updateFrame() {
  glfwPollEvents();
  TextureView nextTexture = mSwapChain.getCurrentTextureView();
  if (!nextTexture) {
    terminateSwapChain();
    initSwapChain();
    nextTexture = mSwapChain.getCurrentTextureView();
  }

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
  renderPassColorAttachment.clearValue = Color{0.05, 0.05, 0.05, 1.0};
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

  renderPass.setPipeline(mRenderPipeline);

  // State Machine For Rendering different meshes depending on which mode the
  // user has chosen
  if (isQuaternion || isSO3) {
    adjustView(-0.25, 0.0, -2.0);
    // Update uniform buffer
    // Update view matrix
    angle1 = static_cast<float>(glfwGetTime());
    R1 = glm::rotate(mat4x4(1.0), angle1, vec3(0.0, 0.0, 1.0));
    mUniforms.modelMatrix = R1 * T1 * S;
    mQueue.writeBuffer(mUniformBuffer, offsetof(Uniform, modelMatrix),
                       &mUniforms.modelMatrix, sizeof(Uniform::modelMatrix));
  } else if (isLieAlgebra) {
    adjustView(-1.0, 0.0, -7.0);
    // Left hand side matrix
    Eigen::Matrix3d lhsSO3;
    lhsSO3 << l100, l101, l102, l110, l111, l112, l120, l121, l122;

    // Right hand side matrix
    Eigen::Matrix3d rhsSO3;
    rhsSO3 << r100, r101, r102, r110, r111, r112, r120, r121, r122;

    // Depending on the desired operation
    Eigen::Vector3d desired = Eigen::Vector3d::Zero();
    if (isSub) {
      // This is lhs * (rhs inverse) bc orthogonal matrices transposed are their
      // own inverse
      Eigen::Matrix3d composed = lhsSO3 * rhsSO3.transpose();
      desired = LieAlgebra::logarithmicMap(composed);
    }

    Eigen::Vector3d v = Eigen::Vector3d::Zero();
    v.x() = desired.x();
    v.y() = -1 * desired.y();
    v.z() = desired.z();

    Eigen::HouseholderQR<Eigen::Matrix3d> qr;
    Eigen::Matrix3d vectorMatrix;
    vectorMatrix << v.x(), v.x(), v.x(), v.y(), v.y(), v.y() + 1, v.z(),
        v.z() + 1, v.z(), qr.compute(vectorMatrix);

    Eigen::Matrix3d reorderMatrix = Eigen::Matrix3d::Zero();
    reorderMatrix(2, 1) = 1;
    reorderMatrix(1, 0) = 1;
    reorderMatrix(0, 2) = 1;

    Eigen::Matrix3d rotation = qr.householderQ() * reorderMatrix;

    // Make sure the signs of the vectors have not been changed by QR decomp
    if (getSign(rotation.coeff(0, 2)) != getSign(v.coeff(0, 0)) ||
        getSign(rotation.coeff(1, 2)) != getSign(v.coeff(1, 0)) ||
        getSign(rotation.coeff(2, 2)) != getSign(v.coeff(2, 0))) {
      rotation(0, 2) *= -1;
      rotation(1, 2) *= -1;
      rotation(2, 2) *= -1;
      rotation(0, 1) *= -1;
      rotation(1, 1) *= -1;
      rotation(2, 1) *= -1;
    }

    mZScalar = v.norm();

    rotationGLM = glm::transpose(
        mat4x4(rotation.coeff(0, 0), rotation.coeff(0, 1), rotation.coeff(0, 2),
               0, rotation.coeff(1, 0), rotation.coeff(1, 1),
               rotation.coeff(1, 2), 0, rotation.coeff(2, 0),
               rotation.coeff(2, 1), rotation.coeff(2, 2), 0, 0, 0, 0, 1));

    mQueue.writeBuffer(mUniformBuffer,
                       2 * mUniformStride + offsetof(Uniform, zScalar),
                       &mZScalar, sizeof(Uniform::zScalar));

    mQueue.writeBuffer(mUniformBuffer,
                       2 * mUniformStride + offsetof(Uniform, rotation),
                       &rotationGLM, sizeof(Uniform::rotation));
  }

  // Determine which objects to render based on which mode we are in
  std::vector<size_t> toRender;
  if (isQuaternion || isSO3) {
    toRender = {0, 1};
  } else if (isLieAlgebra) {
    toRender = {1, 2};
  }

  // Set binding group
  uint32_t dynamicOffset = 0;
  for (size_t i : toRender) {
    dynamicOffset = mUniformStride * mUniformIndices[i];

    renderPass.setVertexBuffer(0, mVertexBuffers[i], 0,
                               mVertexDatas[i].size() *
                                   sizeof(VertexAttributes)); // changed

    // Set binding group
    renderPass.setBindGroup(0, mBindGroup, 1, &dynamicOffset);

    renderPass.draw(mIndexCounts[i], 1, 0, 0); // changed
  }

  // We add the GUI drawing commands to the render pass
  updateGUI(renderPass);

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

  mSwapChain.present();

#ifdef WEBGPU_BACKEND_DAWN
  // Check for pending error callbacks
  mDevice.tick();
#endif

  std::this_thread::sleep_until(mEndFrame);
  mBeginFrame = mEndFrame;
  mEndFrame = mBeginFrame + inverseFPS;
}

// This function runs in the LIFO order like regular destructors
void Rendering::terminate() {
  terminateGUI();
  terminateBindGroup();
  terminateUniforms();
  terminateGeometry();
  terminateRenderPipeline();
  terminateDepthBuffer();
  terminateSwapChain();
  terminateQueue();
  terminateAapterAndDevice();
  terminateGLFW();
}

bool Rendering::isOpen() { return !glfwWindowShouldClose(mWindow); }

void Rendering::initGLFW() {
  // Initialize GLFW
  if constexpr (isDebug) {
    std::cout << "Initializing GLFW..." << std::endl;
  }
  std::vector<std::pair<int, int>> args{{GLFW_CLIENT_API, GLFW_NO_API},
                                        {GLFW_RESIZABLE, GLFW_FALSE}};
  GLFW::init(args);
  if constexpr (isDebug) {
    std::cout << "GLFW Initialized" << std::endl;
  }

  // Create window
  mWindow =
      GLFW::createWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Orientation Visualizer");
}

void Rendering::terminateGLFW() { GLFW::terminate(); }

void Rendering::initAdapterAndDevice() {
  if constexpr (isDebug) {
    std::cout << "Initializing Adapter..." << std::endl;
  }
  mSurface = glfwGetWGPUSurface(mInstance, mWindow);
  RequestAdapterOptions adapterOpts{};
  adapterOpts.compatibleSurface = mSurface;
  Adapter adapter = mInstance.requestAdapter(adapterOpts);
  if constexpr (isDebug) {
    std::cout << "Adapter: " << adapter << std::endl;
  }
  adapter.getLimits(&mSupportedLimits);

  if constexpr (isDebug) {
    std::cout << "Initializing Device..." << std::endl;
  }
  RequiredLimits requiredLimits = Default;
  requiredLimits.limits.maxVertexAttributes = 4;
  requiredLimits.limits.maxVertexBuffers = 1;
  requiredLimits.limits.maxBufferSize = 150000 * sizeof(VertexAttributes);
  requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
  requiredLimits.limits.minStorageBufferOffsetAlignment =
      mSupportedLimits.limits.minStorageBufferOffsetAlignment;
  requiredLimits.limits.minUniformBufferOffsetAlignment =
      mSupportedLimits.limits.minUniformBufferOffsetAlignment;
  requiredLimits.limits.maxInterStageShaderComponents = 8;
  requiredLimits.limits.maxBindGroups = 2;
  requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
  requiredLimits.limits.maxUniformBufferBindingSize = sizeof(Uniform);
  requiredLimits.limits.maxTextureDimension1D = 2048;
  requiredLimits.limits.maxTextureDimension2D = 2048;
  requiredLimits.limits.maxTextureArrayLayers = 1;
  requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
  requiredLimits.limits.maxSamplersPerShaderStage = 1;
  requiredLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;

  DeviceDescriptor deviceDesc;
  deviceDesc.label = "WGPU Device";
  deviceDesc.requiredFeaturesCount = 0;
  deviceDesc.requiredLimits = &requiredLimits;
  deviceDesc.defaultQueue.label = "The default queue";
  mDevice = adapter.requestDevice(deviceDesc);
  if constexpr (isDebug) {
    std::cout << "Device: " << mDevice << std::endl;
  }

  // Device error callback for more descriptive
  mErrorCallback = mDevice.setUncapturedErrorCallback(
      [](ErrorType type, char const *message) {
        std::cout << "Error Type: " << type;
        if (message)
          std::cout << message << "\n";
      });
  adapter.release();
}

void Rendering::terminateAapterAndDevice() {
  mDevice.release();
  mSurface.release();
}

void Rendering::initQueue() {
  // Initialize Queue
  if constexpr (isDebug) {
    std::cout << "Initializing Queue..." << std::endl;
  }
  mQueue = mDevice.getQueue();
  if (!mQueue) {
    std::cerr << "Queue did not initialize properly!" << std::endl;
    throw std::runtime_error("Queue did not initialize properly!");
  }
  if constexpr (isDebug) {
    std::cout << "Queue: " << mQueue << std::endl;
  }
}

void Rendering::terminateQueue() { mQueue.release(); }

void Rendering::initSwapChain() {
  // Get the current size of the window's framebuffer:
  if constexpr (isDebug) {
    std::cout << "Initializing Swap Chain..." << std::endl;
  }
  SwapChainDescriptor swapChainDesc;
  swapChainDesc.width = static_cast<uint32_t>(WINDOW_WIDTH);
  swapChainDesc.height = static_cast<uint32_t>(WINDOW_HEIGHT);
  swapChainDesc.usage = TextureUsage::RenderAttachment;
  swapChainDesc.format = mSwapChainFormat;
  swapChainDesc.presentMode = PresentMode::Immediate;
  mSwapChain = mDevice.createSwapChain(mSurface, swapChainDesc);
  if (!mSwapChain) {
    std::cerr << "Swap Chain did not initialize properly!" << std::endl;
    throw std::runtime_error("Swap Chain did not initialize properly!");
  }
  if constexpr (isDebug) {
    std::cout << "Swap Chain: " << mSwapChain << std::endl;
  }
}

void Rendering::terminateSwapChain() { mSwapChain.release(); }

void Rendering::initDepthBuffer() {
  // Create the depth texture
  if constexpr (isDebug) {
    std::cout << "Depth Buffer..." << std::endl;
  }
  TextureDescriptor depthTextureDesc;
  depthTextureDesc.dimension = TextureDimension::_2D;
  depthTextureDesc.format = mDepthTextureFormat;
  depthTextureDesc.mipLevelCount = 1;
  depthTextureDesc.sampleCount = 1;
  depthTextureDesc.size = {static_cast<uint32_t>(WINDOW_WIDTH),
                           static_cast<uint32_t>(WINDOW_HEIGHT), 1};
  depthTextureDesc.usage = TextureUsage::RenderAttachment;
  depthTextureDesc.viewFormatCount = 1;
  depthTextureDesc.viewFormats = (WGPUTextureFormat *)&mDepthTextureFormat;
  mDepthTexture = mDevice.createTexture(depthTextureDesc);
  if (!mDepthTexture) {
    std::cerr << "Depth Texture did not initialize properly!" << std::endl;
    throw std::runtime_error("Depth Texture did not initialize properly!");
  }
  if constexpr (isDebug) {
    std::cout << "Depth Texture: " << mDepthTexture << std::endl;
  }
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
  if (!mDepthTextureView) {
    std::cerr << "Depth Texture View did not initialize properly!" << std::endl;
    throw std::runtime_error("Depth Texture View did not initialize properly!");
  }
  if constexpr (isDebug) {
    std::cout << "Depth Texture View: " << mDepthTextureView << std::endl;
  }
}

void Rendering::terminateDepthBuffer() {
  mDepthTextureView.release();
  mDepthTexture.destroy();
  mDepthTexture.release();
}

void Rendering::writeRotation() {
  if (isQuaternion) {
    SE3 = transpose(mat4x4(
        2 * (std::pow(q0, 2) + std::pow(q1, 2)) - 1, 2 * (q1 * q2 - q0 * q3),
        2 * (q1 * q3 + q0 * q2), 0, 2 * (q1 * q2 + q0 * q3),
        2 * (std::pow(q0, 2) + std::pow(q2, 2)) - 1, 2 * (q2 * q3 - q0 * q1), 0,
        2 * (q1 * q3 - q0 * q2), 2 * (q2 * q3 + q0 * q1),
        2 * (std::pow(q0, 2) + std::pow(q3, 2)) - 1, 0, 0, 0, 0, 1));
  } else if (isSO3) {
    SE3 = transpose(mat4x4(i00, i01, i02, 0, i10, i11, i12, 0, i20, i21, i22, 0,
                           0, 0, 0, 1));
  }

  mQueue.writeBuffer(mUniformBuffer,
                     mUniformStride + offsetof(Uniform, rotation), &SE3,
                     sizeof(Uniform::rotation));
}

ShaderModule Rendering::loadShaderModule(const std::filesystem::path &path,
                                         Device device) {
  std::ifstream file1(path);
  if (!file1.is_open()) {
    std::cerr << "Shader File did not open properly! " << path.string()
              << std::endl;
    throw std::runtime_error("Shader File did not open");
  }
  // Read in file
  size_t filesize = 0;
  while (file1.good()) {
    file1.get();
    ++filesize;
  }

  // Read in the file
  std::string shaderSourceCode(filesize, '\0');

  std::ifstream file2(path);
  if (!file2.is_open()) {
    std::cerr << "Shader Filedid not initialize properly!" << std::endl;
    throw std::runtime_error("Shader File did not initialize properly");
  }
  file2.read(shaderSourceCode.data(), filesize);

  ShaderModuleWGSLDescriptor shaderModuleDesc;
  shaderModuleDesc.chain.next = nullptr;
  shaderModuleDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
  shaderModuleDesc.code = shaderSourceCode.c_str();
  ShaderModuleDescriptor shaderDesc;
  shaderDesc.nextInChain = &shaderModuleDesc.chain;

  return device.createShaderModule(shaderDesc);
}

void Rendering::initRenderPipeline() {
  // Load the shader module
  if constexpr (isDebug) {
    std::cout << "Shader Module..." << std::endl;
  }
  mShaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", mDevice);
  if constexpr (isDebug) {
    std::cout << "Shader Module: " << mShaderModule << std::endl;
  }
  if constexpr (isDebug) {
    std::cout << "Render Pipeline..." << std::endl;
  }
  RenderPipelineDescriptor pipelineDesc;

  // Describe the attirbutes that the vertices will have
  std::vector<VertexAttribute> vertexAttributes(3);

  // Position
  vertexAttributes[0].shaderLocation = 0;
  vertexAttributes[0].format = VertexFormat::Float32x3;
  vertexAttributes[0].offset = 0;

  // Normal
  vertexAttributes[1].shaderLocation = 1;
  vertexAttributes[1].format = VertexFormat::Float32x3;
  vertexAttributes[1].offset = offsetof(VertexAttributes, normal);

  // Color
  vertexAttributes[2].shaderLocation = 2;
  vertexAttributes[2].format = VertexFormat::Float32x3;
  vertexAttributes[2].offset = offsetof(VertexAttributes, color);

  VertexBufferLayout vertexBufferLayout;
  vertexBufferLayout.attributeCount =
      static_cast<uint32_t>(vertexAttributes.size());
  vertexBufferLayout.attributes = vertexAttributes.data();
  vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
  vertexBufferLayout.stepMode = VertexStepMode::Vertex;

  pipelineDesc.vertex.bufferCount =
      1; // Since I'm just using one buffer with offsets this stays at 1
  pipelineDesc.vertex.buffers = &vertexBufferLayout;

  pipelineDesc.vertex.module = mShaderModule;
  pipelineDesc.vertex.entryPoint =
      "vs_main"; // This is the function it will run from the shader module to
                 // process vertices
  pipelineDesc.vertex.constantCount = 0;
  pipelineDesc.vertex.constants = nullptr;

  pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
  pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
  pipelineDesc.primitive.frontFace =
      FrontFace::CCW; // Going around the triangle counter clockwise will tell
                      // which is the front face
  pipelineDesc.primitive.cullMode = CullMode::None;

  FragmentState fragmentState;
  pipelineDesc.fragment = &fragmentState;
  fragmentState.module = mShaderModule;
  fragmentState.entryPoint =
      "fs_main"; // This is the function it will run from the shader module to
                 // process fragments from the vertex sahder
  fragmentState.constantCount = 0;
  fragmentState.constants = nullptr;

  // Outline how the pipeline should deal with colors
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
  mBindingLayout.buffer.minBindingSize = sizeof(Uniform);
  mBindingLayout.buffer.hasDynamicOffset = true;

  // Create a bind group layout
  BindGroupLayoutDescriptor bindGroupLayoutDesc{};
  bindGroupLayoutDesc.entryCount = bindGroupLayoutDescEntryCount;
  bindGroupLayoutDesc.entries = &mBindingLayout;
  mBindGroupLayout = mDevice.createBindGroupLayout(bindGroupLayoutDesc);

  // Create the pipeline layout
  PipelineLayoutDescriptor layoutDesc{};
  layoutDesc.bindGroupLayoutCount = 1;
  layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout *)&mBindGroupLayout;
  PipelineLayout layout = mDevice.createPipelineLayout(layoutDesc);
  pipelineDesc.layout = layout;

  mRenderPipeline = mDevice.createRenderPipeline(pipelineDesc);
  if constexpr (isDebug) {
    std::cout << "Shader Module: " << mRenderPipeline << std::endl;
  }
}

void Rendering::terminateRenderPipeline() {
  mRenderPipeline.release();
  mShaderModule.release();
  mBindGroupLayout.release();
}

void Rendering::loadGeometry(const std::string &url, int uniformID) {
  if constexpr (isDebug) {
    std::cout << "Loading " << url << "..." << std::endl;
  }

  // Make sure that we are in boudns on uniforms
  if (uniformID > MAX_NUM_UNIFORMS) {
    std::cerr << "Could not load Mesh! UniformID Exceedes Buffer Size"
              << std::endl;
    throw std::runtime_error(
        "Could not load Mesh! Mesh Count Has Exceeded Buffer Size");
  }

  // Create a new vector for pushing back all of the vertices
  mVertexDatas.push_back(std::vector<VertexAttributes>());
  mUniformIndices.push_back(uniformID);

  // Read in the scenes of meshes
  Assimp::Importer MeshImporter;
  const aiScene *scene =
      aiImportFile((std::string(RESOURCE_DIR) + url).c_str(),
                   aiProcessPreset_TargetRealtime_MaxQuality);

  if (!scene) {
    std::cerr << "Could not load scene! ASSIMP ERROR: " << aiGetErrorString()
              << std::endl;
    throw std::runtime_error("Could not load scene!");
  }

  // Load all of the meshes
  std::uint32_t index = 0;
  for (std::uint32_t meshIdx = 0u; meshIdx < scene->mNumMeshes; ++meshIdx) {
    aiMesh *mesh = scene->mMeshes[meshIdx];

    // Grab the color of the mesh
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    aiColor4D color;
    aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color);

    for (std::uint32_t faceIdx = 0u; faceIdx < mesh->mNumFaces; ++faceIdx) {
      // Put the vertices in the format the webgpu/rendering pipelin is looking
      // for
      for (int i = 0; i < 3; ++i) {
        std::uint32_t vertIdx = mesh->mFaces[faceIdx].mIndices[i];
        aiVector3D vertex = mesh->mVertices[vertIdx];
        aiVector3D normal = mesh->mNormals[vertIdx];

        // Create a new place to load all of the vertex attributes
        mVertexDatas[mVertexDatas.size() - 1].push_back(VertexAttributes());

        // Position
        mVertexDatas[mVertexDatas.size() - 1][index].position = {
            vertex.x, vertex.z, vertex.y};

        // Normal
        mVertexDatas[mVertexDatas.size() - 1][index].normal = {
            normal.x, normal.z, normal.y};

        // Color
        mVertexDatas[mVertexDatas.size() - 1][index].color = {color.r, color.g,
                                                              color.b};

        index++;
      }
    }
  }

  aiReleaseImport(scene);

  if (mVertexDatas[mVertexDatas.size() - 1].size() *
          sizeof(decltype(mVertexDatas[mVertexDatas.size() - 1][0])) >
      MAX_BUFFER_SIZE) {
    std::cerr << "Could not load geometry! Mesh Of Size: "
              << mVertexDatas[mVertexDatas.size() - 1].size() *
                     sizeof(decltype(mVertexDatas[mVertexDatas.size() - 1][0]))
              << " Is Too Large For Buffer Of Size " << MAX_BUFFER_SIZE
              << std::endl;
    throw std::runtime_error("Could not load geometry! Mesh Too Large");
  }

  initVertexBuffer();
}

void Rendering::initVertexBuffer() {
  // Create vertex buffer
  BufferDescriptor bufferDesc;
  bufferDesc.size = mVertexDatas[mVertexDatas.size() - 1].size() *
                    sizeof(VertexAttributes); // changed
  if (mVertexDatas[mVertexDatas.size() - 1].size() *
          sizeof(decltype(mVertexDatas[mVertexDatas.size() - 1][0])) >
      MAX_BUFFER_SIZE) {
    std::cerr << "Could not load geometry! Mesh Of Size: "
              << mVertexDatas[mVertexDatas.size() - 1].size() *
                     sizeof(decltype(mVertexDatas[mVertexDatas.size() - 1][0]))
              << " Is Too Large For Buffer Of Size " << MAX_BUFFER_SIZE
              << std::endl;
    throw std::runtime_error("Could not load geometry! Mesh Too Large");
  }
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
  bufferDesc.mappedAtCreation = false;
  mVertexBuffers.push_back(mDevice.createBuffer(bufferDesc));

  // Write all of the vertex data onto the new buffer
  mQueue.writeBuffer(mVertexBuffers[mVertexBuffers.size() - 1], 0,
                     mVertexDatas[mVertexDatas.size() - 1].data(),
                     bufferDesc.size); // changed

  // Push back the number of vertices in the vector
  mIndexCounts.push_back(
      static_cast<int>(mVertexDatas[mVertexDatas.size() - 1].size()));
}

void Rendering::terminateGeometry() {
  for (auto buff : mVertexBuffers) {
    buff.destroy();
    buff.release();
  }
}

void Rendering::terminateUniforms() {
  mUniformBuffer.destroy();
  mUniformBuffer.release();
}

void Rendering::initBindGroup() {
  // Create a binding
  if constexpr (isDebug) {
    std::cout << "Bind Group..." << std::endl;
  }
  BindGroupEntry binding;
  binding.binding = 0;
  binding.buffer = mUniformBuffer;
  binding.offset = 0;
  binding.size = sizeof(Uniform);

  BindGroupDescriptor bindGroupDesc;
  bindGroupDesc.layout = mBindGroupLayout;
  bindGroupDesc.entryCount = 1;
  bindGroupDesc.entries = &binding;
  mBindGroup = mDevice.createBindGroup(bindGroupDesc);

  if constexpr (isDebug) {
    std::cout << "Bind Group: " << mBindGroup << std::endl;
  }
}

void Rendering::terminateBindGroup() { mBindGroup.release(); }

void Rendering::initUniform(int index, const mat4x4 &rotation, float x, float y,
                            float z) {
  // View from which the
  vec3 focalPoint(x, y, z);

  // Rotate the object
  angle1 = 2.0f;

  // Rotate the view point
  angle2 = 3.0f * M_PI / 4.0f;

  S = glm::scale(mat4x4(1.0), vec3(0.3f));
  T1 = mat4x4(1.0);
  R1 = glm::rotate(mat4x4(1.0), angle1, vec3(0.0, 0.0, 1.0));
  mUniforms.modelMatrix = R1 * T1 * S;

  mat4x4 R2 = glm::rotate(mat4x4(1.0), -angle2, vec3(1.0, 0.0, 0.0));
  mat4x4 T2 = glm::translate(mat4x4(1.0), -focalPoint);
  mUniforms.viewMatrix = T2 * R2;

  float ratio =
      static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
  float focalLength = 2.5;
  float near = 0.1f;
  float far = 10.0f;
  float divider = 1 / (focalLength * (far - near));
  mUniforms.projectionMatrix = transpose(
      mat4x4(1.0, 0.0, 0.0, 0.0, 0.0, ratio, 0.0, 0.0, 0.0, 0.0, far * divider,
             -far * near * divider, 0.0, 0.0, 1.0 / focalLength, 0.0));

  mUniforms.zScalar = 1.0f;
  mUniforms.color = {0.0f, 1.0f, 0.4f, 1.0f};
  mUniforms.rotation = rotation;

  mQueue.writeBuffer(mUniformBuffer, index * mUniformStride, &mUniforms,
                     sizeof(Uniform));
}

void Rendering::initUniformBuffer() {
  BufferDescriptor bufferDesc;
  bufferDesc.size = MAX_BUFFER_SIZE;
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
  bufferDesc.mappedAtCreation = false;
  // Create uniform buffer
  mUniformStride =
      std::ceil(static_cast<double>(sizeof(Uniform)) /
                mSupportedLimits.limits.minUniformBufferOffsetAlignment) *
      mSupportedLimits.limits.minUniformBufferOffsetAlignment;
  bufferDesc.size = (MAX_NUM_UNIFORMS - 1) * mUniformStride + sizeof(Uniform);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  bufferDesc.mappedAtCreation = false;
  mUniformBuffer = mDevice.createBuffer(bufferDesc);
}

void Rendering::adjustView(float x, float y, float z) {
  for (size_t i = 0; i < MAX_NUM_UNIFORMS; ++i) {
    initUniform(
        i, transpose(mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)), x,
        y, z);
  }
}
