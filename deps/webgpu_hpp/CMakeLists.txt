cmake_minimum_required(VERSION 3.29.0)

project(
	webgpu_hpp
	VERSION 1.0.0
	LANGUAGES CXX C
)

add_library(webgpu_hpp INTERFACE headers/webgpu.hpp)
