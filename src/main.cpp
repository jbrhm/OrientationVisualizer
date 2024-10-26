#define WEBGPU_CPP_IMPLEMENTATION

#include "LieAlgebra.hpp"
#include "Rendering.hpp"
#include <iostream>

int main(int, char **) {
  Rendering viz;

  while (viz.isOpen()) {
    viz.updateFrame();
  }

  viz.terminate();
  return 0;
}
