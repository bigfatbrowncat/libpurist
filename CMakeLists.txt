cmake_minimum_required(VERSION 3.25)

# Globally adding libasan for debug build
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize=address>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize=address>)

project(purist)

add_executable(purist 
    main.cpp
    modeset.cpp
)

target_include_directories(purist PRIVATE /usr/include/libdrm)
target_link_libraries(purist drm)