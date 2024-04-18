cmake_minimum_required(VERSION 3.29.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
	LearnWebGPU
	VERSION 0.1.0
	LANGUAGES CXX C
)

set(srcs src/main.cpp src/GLFW.cpp )

add_executable(App ${srcs})

set_target_properties(App PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
	target_compile_options(App PRIVATE /W4)
else()
	target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

if(XCODE)
	set_target_properties(App PROPERTIES
		XCODE_GENERATE_SCHEME ON
		XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
endif()

# Add Dependencies
add_subdirectory(deps/glfw-3.4)

add_subdirectory(deps/glfw3webgpu)

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("deps/dawn" EXCLUDE_FROM_ALL)
target_link_libraries(App PRIVATE webgpu_cpp webgpu_dawn webgpu_glfw)

add_subdirectory(deps/webgpu_hpp)

# Link against deps
target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu webgpu_hpp)

# Include deps includes
target_include_directories(App INTERFACE webgpuhpp)
