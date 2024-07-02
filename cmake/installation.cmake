# Install Application
install(TARGETS Viz DESTINATION bin)

# Install Shared Library Dependencies
install(FILES ${CMAKE_BINARY_DIR}/libwgpu_native.so DESTINATION lib)
install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES ./Viz
            RESOLVED_DEPENDENCIES_VAR _r_deps
        )
        foreach(_file ${_r_deps})
            file(INSTALL
                DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
                TYPE SHARED_LIBRARY
                FOLLOW_SYMLINK_CHAIN
                FILES "${_file}"
            )
        endforeach()
      ]])

install(DIRECTORY ${CMAKE_BINARY_DIR}/../resources DESTINATION .)