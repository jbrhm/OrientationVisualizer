# Install Prefix
set(CMAKE_INSTALL_PREFIX "/opt/OrientationVisualizer")

# Install Application
install(TARGETS Viz DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

# Install Shared Library Dependencies
install(FILES ${CMAKE_BINARY_DIR}/libwgpu_native.so DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

# Installs all of the runtime deps
set(installAllDeps OFF)

if(installAllDeps)
	install(CODE [[
			file(GET_RUNTIME_DEPENDENCIES
				EXECUTABLES ./Viz
				RESOLVED_DEPENDENCIES_VAR _r_deps
			)

			# CMake deps we dont want to package

			foreach(_file ${_r_deps})
				file(INSTALL
					DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
					TYPE SHARED_LIBRARY
					FOLLOW_SYMLINK_CHAIN
					FILES "${_file}"
				)
			endforeach()
		  ]])
endif()

install(DIRECTORY ${CMAKE_BINARY_DIR}/../resources DESTINATION "${CMAKE_INSTALL_PREFIX}")
