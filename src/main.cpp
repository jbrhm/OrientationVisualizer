#define WEBGPU_CPP_IMPLEMENTATION

#include "Application.h"


int main(int, char**) {
	Application app;
	if (!app.onInit()) return 1;

	while (app.isRunning()) {
		app.onFrame();
	}

	app.onFinish();
	return 0;
}
