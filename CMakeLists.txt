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

option(DEV_MODE "Set up development helper settings" ON)

add_executable(Viz
	${srcs}
) 

# Build Configurations
set(isDEBUG FALSE)
if(isDEBUG)
	target_compile_options(Viz PRIVATE -g3)
	target_compile_definitions(Viz PRIVATE
		DEBUG="ON"
	)
else()
	target_compile_options(Viz PRIVATE -O3)
endif()

# Define the resources directory
target_compile_definitions(Viz PRIVATE
	RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)

target_include_directories(Viz PRIVATE .)

target_link_libraries(Viz PRIVATE glfw webgpu glfw3webgpu glm assimp imgui eigen)

set_target_properties(Viz PROPERTIES
	CXX_STANDARD 17
	VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
)
target_treat_all_warnings_as_errors(Viz)
target_copy_webgpu_binaries(Viz)

# Ignore a warning that GLM requires to bypass
if (MSVC)
	# Disable warning C4201: nonstandard extension used: nameless struct/union
	target_compile_options(Viz PUBLIC /wd4201)
endif (MSVC)
