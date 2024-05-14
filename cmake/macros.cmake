macro(set_source_directory var dir)
    file(GLOB_RECURSE ${var} CONFIGURE_DEPENDS ${dir}/*)
endmacro()