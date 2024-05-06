macro(set_source_directory var dir)
    file(GLOB_RECURSE ${var} ${dir}/*)
endmacro()