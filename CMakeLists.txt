cmake_minimum_required(VERSION 3.25)
project(purist)

add_executable(purist main.cpp)

target_include_directories(purist PRIVATE /usr/include/libdrm)
target_link_libraries(purist drm)