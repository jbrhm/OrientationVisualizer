cmake_minimum_required(VERSION 3.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(eigen-3.4.0 SYSTEM EXCLUDE_FROM_ALL)
add_subdirectory(manif SYSTEM EXCLUDE_FROM_ALL)

project(lol)

add_executable(lol main.cpp)

target_include_directories(lol PRIVATE .)

target_link_libraries(lol PRIVATE manif)