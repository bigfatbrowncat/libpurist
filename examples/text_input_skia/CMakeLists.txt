embed_resources(NotoSansFont 
    fonts/noto-sans/NotoSans-Regular.ttf
    fonts/noto-sans/NotoSansHebrew-Regular.ttf
)

add_executable(text_input_skia
    text_input_skia.cpp
    ${NotoSansFont}
)
target_link_libraries(text_input_skia PUBLIC 
    purist_graphics_skia
    skparagraph
    skunicode_core
    skunicode_icu
    skshaper
)

add_dependencies(text_input_skia purist_graphics_skia)
