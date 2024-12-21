cmake_minimum_required(VERSION 3.25)

include(globals.cmake)

project(purist)

set(purist_third_party "${purist_SOURCE_DIR}/third-party")

add_subdirectory(libpurist_graphics)
add_subdirectory(libpurist_graphics_skia)

add_subdirectory(examples/blinking_screen)
add_subdirectory(examples/color_rectangle_skia)
