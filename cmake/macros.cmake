macro(set_source_directory var dir)
    file(GLOB_RECURSE ${var} CONFIGURE_DEPENDS ${dir}/*c*)
endmacro()

macro(set_include_directory var dir)
	file(GLOB_RECURSE ${var} CONFIGURE_DEPENDS ${dir}*.h*)
endmacro()
