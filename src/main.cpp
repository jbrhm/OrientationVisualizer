#define WEBGPU_CPP_IMPLEMENTATION

#include "Application.h"
#include "LieAlgebra.hpp"
#include <iostream>


int main(int, char**) {
	// Lie Algebra testing
	Eigen::Matrix3d so3;
	so3 << 	-0.8, 0, 0.6, 
			0.36, -0.8, 0.48,
			0.48, 0.6, 0.64;
	Eigen::Vector3d v = LieAlgebra::logarithmicMap(so3);

	std::cout << v << std::endl;

	Application app;
	if (!app.onInit()) return 1;

	while (app.isRunning()) {
		app.onFrame();
	}

	app.onFinish();
	return 0;
}
