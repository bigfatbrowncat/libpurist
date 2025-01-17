embed_resources(NotoSansFont 
    fonts/noto-sans/NotoSans-Regular.ttf
    fonts/noto-sans/NotoSansHebrew-Regular.ttf
)

add_executable(color_rectangle_skia
    color_rectangle_skia.cpp
    ${NotoSansFont}
)
target_link_libraries(color_rectangle_skia PUBLIC 
    purist_graphics_skia
    skparagraph
    skunicode_core
    skunicode_icu
    skshaper
)

add_dependencies(color_rectangle_skia purist_graphics_skia)
