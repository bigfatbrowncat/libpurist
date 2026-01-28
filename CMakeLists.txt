cmake_minimum_required(VERSION 3.18)

include(globals.cmake)

project(purist)

set(purist_third_party "${purist_SOURCE_DIR}/third-party")
set(CMAKE_PREFIX_PATH "${purist_third_party}/prefix")

include_directories(${purist_third_party}/embed-resource)
add_subdirectory(${purist_third_party}/embed-resource)

add_subdirectory(libpurist_graphics)
add_subdirectory(libpurist_graphics_skia)

add_subdirectory(examples/blinking_screen)
add_subdirectory(examples/text_input_skia)
add_subdirectory(examples/term_skia)
