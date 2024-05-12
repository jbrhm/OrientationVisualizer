#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define WEBGPU_CPP_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION // add this to exactly 1 of your C++ files

#include "Rendering.hpp"

int main (int, char**) {
	Rendering r;
	r.renderTexture();
	return 0;
}