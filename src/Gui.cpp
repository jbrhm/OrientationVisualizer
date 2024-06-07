#include "Gui.hpp"
#include <backends/imgui_impl_glfw.h>
#include <stdexcept>
#include <backends/imgui_impl_wgpu.h>

using namespace std;

void MySaveFunction(){

}

Gui::Gui(wgpu::TextureFormat depthStencilFormat, wgpu::TextureFormat renderTargetFormat, wgpu::Device& device, GLFWwindow* window){
	// Create a window called "My First Tool", with a menu bar.
	mWindow = window;

	if(!mWindow){
		throw std::runtime_error("Window did not initialize properly");
	}

	// Setup Imgui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOther(mWindow, true);
	
	initInfo.DepthStencilFormat = depthStencilFormat;
	initInfo.RenderTargetFormat = renderTargetFormat;
	initInfo.Device = device;
	initInfo.NumFramesInFlight = 3;
	ImGui_ImplWGPU_Init(&initInfo);

	// Get the sizing of the current monitor
	int x, y, w, h;
	glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &x, &y, &w, &h);
	cout << "The monitor's dimensions are (x,y,w,h): " << x << ", " << y << ", " << w << ", " << h << endl;

	// Imgui styling
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	float scale = h > 1500 ? 2.0f : 1.0f;
	io.FontGlobalScale = scale;
	style.ScaleAllSizes(scale);
}

void Gui::renderGui(wgpu::RenderPassEncoder& renderPass){
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Create the items to add the imgui
	ImGui::Begin("Settings", nullptr);
	float mTargetUpdateRate = 0;
	ImGui::SliderFloat("Target FPS", &mTargetUpdateRate, 5.0f, 1000.0f);
	ImGui::End();

	ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

Gui::~Gui(){}
