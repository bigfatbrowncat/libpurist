cmake_minimum_required(VERSION 3.25)

# Globally adding libasan
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

project(purist)

add_executable(purist main.cpp)

target_include_directories(purist PRIVATE /usr/include/libdrm)
target_link_libraries(purist drm)