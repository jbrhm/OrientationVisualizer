cmake_minimum_required(VERSION 3.27)

project(
	OrientationVisualizer
	VERSION 0.1.0
	LANGUAGES CXX C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/utils.cmake)

include(cmake/deps.cmake)

include(cmake/macros.cmake)

set_source_directory(srcs src)

set_include_directory(includes src)

add_executable(Viz
	${srcs}
) 

# Build Configurations
add_library(VisualizerCompileOptions INTERFACE)

set(isDEBUG OFF)

target_compile_features(VisualizerCompileOptions INTERFACE cxx_std_20)

target_compile_options(VisualizerCompileOptions INTERFACE $<IF:$<BOOL:${isDEBUG}>,-g3,-O3>)

# Define the resources directory
target_compile_definitions(Viz PRIVATE
	RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
	$<$<BOOL:${isDEBUG}>:DEBUG="ON">
)

target_include_directories(Viz PRIVATE ${includes})

target_link_libraries(Viz PRIVATE VisualizerCompileOptions glfw webgpu glfw3webgpu glm assimp imgui eigen)

set_target_properties(Viz PROPERTIES
	VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
)

target_treat_all_warnings_as_errors(Viz)
target_copy_webgpu_binaries(Viz)

# Ignore a warning that GLM requires to bypass
if (MSVC)
	# Disable warning C4201: nonstandard extension used: nameless struct/union
	target_compile_options(Viz PUBLIC /wd4201)
endif (MSVC)

install(TARGETS Viz DESTINATION bin)
message(${CMAKE_BINARY_DIR})
install(FILES cmake/viz.conf DESTINATION /etc/ld.so.conf.d)
install(FILES ${CMAKE_BINARY_DIR}/libwgpu_native.so DESTINATION lib)

include(cmake/Packing.cmake)