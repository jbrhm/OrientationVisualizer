#include "Application.h"

using namespace wgpu;
using glm::mat4x4;
using glm::vec4;
using glm::vec3;

void Application::initGUI() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO();

    // Init Imgui GLFW
	ImGui_ImplGlfw_InitForOther(mWindow, true);
	ImGui_ImplWGPU_InitInfo initInfo;
	initInfo.DepthStencilFormat = mDepthTextureFormat;
	initInfo.RenderTargetFormat = mSwapChainFormat;
	initInfo.Device = mDevice;
	initInfo.NumFramesInFlight = 1;

    // Init Imgui WGPU
	ImGui_ImplWGPU_Init(&initInfo);

	// Adjust sylistic things
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = imguiScale;
}

void Application::terminateGUI() {
	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplWGPU_Shutdown();
}

void Application::updateGUI(RenderPassEncoder renderPass) {
    // Grab next frams
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Build our UI
	{
		ImGui::Begin("Orientation Visualizer");
		ImGui::Text("Which input would you like?");
		ImGui::Checkbox("Quaternion: ", &isQuaternion);
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
		
		// Refresh rate
		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}