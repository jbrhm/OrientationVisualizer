#pragma once

#include <imgui.h>
#include <math.h>
#include <memory>

#include "GLFW.hpp"

class Gui {
private:
	std::shared_ptr<GLFWwindow*> mWindow;
public:
	Gui();

	~Gui();
};

