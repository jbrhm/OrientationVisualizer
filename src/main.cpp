#define WEBGPU_CPP_IMPLEMENTATION

#include "Rendering.hpp"
#include "LieAlgebra.hpp"
#include <iostream>


int main(int, char**) {
	Rendering viz;

	while (viz.isOpen()) {
		viz.updateFrame();
	}

	viz.terminate();
	return 0;
}
