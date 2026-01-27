embed_resources(HackNerdFontMono 
    fonts/DejaVuSansMono/DejaVuSansMono.ttf
    fonts/HackNerdFontMono/HackNerdFontMono-Regular.ttf
    fonts/HackNerdFontMono/HackNerdFontMono-Bold.ttf
    fonts/HackNerdFontMono/HackNerdFontMono-Italic.ttf
    fonts/HackNerdFontMono/HackNerdFontMono-BoldItalic.ttf
)

add_executable(term_skia
    term_skia.cpp
    TermSubprocess.cpp
    TermSubprocess.h
    VTermWrapper.cpp
    VTermWrapper.h
    SkiaTermEmulator.cpp
    SkiaTermEmulator.h
    ${HackNerdFontMono}
)

target_include_directories(term_skia PUBLIC ../../third-party/libvterm/include)
target_link_directories(term_skia PUBLIC ../../third-party/libvterm)
target_link_libraries(term_skia PUBLIC 
    purist_graphics_skia
#    skparagraph
    skunicode_core
    skunicode_icu
#    skshaper
    vterm
    util
)

add_dependencies(term_skia purist_graphics_skia)

add_subdirectory(app_examples)
