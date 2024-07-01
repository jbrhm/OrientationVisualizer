cmake_minimum_required(VERSION 3.27)

project(webgpuhpp)

add_library(webgpuhpp INTERFACE webgpu.hpp)

target_link_libraries(webgpuhpp INTERFACE webgpu_cpp)

target_include_directories(webgpuhpp INTERFACE .)