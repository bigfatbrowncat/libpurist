embed_resources(HackNerdFontMono 
    fonts/DejaVuSansMono/DejaVuSansMono.ttf
    fonts/HackNerdFontMono/HackNerdFontMono-Regular.ttf
    fonts/HackNerdFontMono/HackNerdFontMono-Bold.ttf
    fonts/HackNerdFontMono/HackNerdFontMono-Italic.ttf
    fonts/HackNerdFontMono/HackNerdFontMono-BoldItalic.ttf
)

add_executable(term_skia
    term_skia.cpp
    ${HackNerdFontMono}
)

target_include_directories(term_skia PUBLIC ../../third-party/libvterm/include)
target_link_directories(term_skia PUBLIC ../../third-party/libvterm/.libs)
target_link_libraries(term_skia PUBLIC 
    purist_graphics_skia
#    skparagraph
    skunicode_core
    skunicode_icu
#    skshaper
    vterm
)

add_dependencies(term_skia purist_graphics_skia)
