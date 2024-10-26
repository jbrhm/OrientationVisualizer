#pragma once

// STL
#include <exception>
#include <iostream>
#include <utility>
#include <vector>

// GLFW
#include <GLFW/glfw3.h>

// The GLFW class handles all of the behind the scenes operations for GLFW
class GLFW {
private:
  // Flag to see if GLFW has been initialized
  inline static bool isGLFWInit = false;

  // Vector to keep track of all windows that GLFW will have to destroy upon
  // termination
  inline static std::vector<GLFWwindow *> windows;

public:
  // Alias for GLFWwindow* so the rest of the program doesn't have to include it
  typedef GLFWwindow *WindowPtr;

  // Initializes GLFW and configures all of the desired arguments
  inline static void init(const std::vector<std::pair<int, int>> &args) {
    if (!isGLFWInit) {
      if (!glfwInit()) {
        std::cerr << "GLFW did not initialize properly!" << std::endl;
        throw std::runtime_error("GLFW did not initialize properly!");
      }
      isGLFWInit = true;
    }
    for (const auto &pair : args) {
      glfwWindowHint(pair.first, pair.second);
    }
  }

  // Creates GLFW windows and keeps track inside of the vector
  inline static WindowPtr createWindow(int width, int height,
                                       const std::string &name,
                                       GLFWmonitor *monitor = nullptr,
                                       GLFWwindow *share = nullptr) {
    GLFWwindow *ptr =
        glfwCreateWindow(width, height, name.c_str(), monitor, share);
    if (!ptr) {
      std::cerr << "Window did not initialize properly!" << std::endl;
      throw std::runtime_error("Window did not initialize properly!");
    }
    windows.emplace_back(ptr);
    return ptr;
  }

  inline static void terminate() {
    for (auto ptr : windows) {
      glfwDestroyWindow(ptr);
    }
    glfwTerminate();
  }
};