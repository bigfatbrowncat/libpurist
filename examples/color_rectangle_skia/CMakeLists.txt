add_executable(color_rectangle_skia
    color_rectangle_skia.cpp
)
target_link_libraries(color_rectangle_skia PUBLIC 
    purist_graphics_skia
)
add_dependencies(color_rectangle_skia purist_graphics_skia)
