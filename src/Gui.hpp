#pragma once

#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>
#include <math.h>
#include <memory>

#include "GLFW.hpp"
#include <webgpu/webgpu.hpp>

class Gui {
private:
	GLFWwindow* mWindow;

	ImGui_ImplWGPU_InitInfo initInfo;
	
public:
	Gui() = default;

	Gui(wgpu::TextureFormat depthStencilFormat, wgpu::TextureFormat renderTargetFormat, wgpu::Device& device, GLFWwindow* window);

	void renderGui(wgpu::RenderPassEncoder& renderPass);

	~Gui();
};

