# Set the name of the package
set(CPACK_PACKAGE_NAME ${PROJECT_NAME} CACHE STRING "The name of my package is OrientationVisualizer")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} is a way to visualize complex orientation representations" CACHE STRING "Package description")

set(CPACK_PACKAGE_VENDOR "jbrhm")

set(CPACK_VERBATIM_VARIABLES YES)

# CPACK_PACKAGE_INSTALL_DIRECTORY is where the actual .deb file will be located
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
# CPACK_OUTPUT_FILE_PREFIX is the prefix for where all of the output files will be. Ex. the .deb file output path will be prefixed with this path
SET(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/_packages")

# This is the directory where the package will installed into when installing the .deb file
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${PROJECT_NAME}")

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_PACKAGE_CONTACT "nah@dontcontactme.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "jbrhm")
