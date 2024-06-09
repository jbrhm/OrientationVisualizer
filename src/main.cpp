#define WEBGPU_CPP_IMPLEMENTATION

#include "Application.h"
#include "LieAlgebra.hpp"
#include <iostream>


int main(int, char**) {
	Application app;

	while (app.isOpen()) {
		app.updateFrame();
	}

	app.terminate();
	return 0;
}
