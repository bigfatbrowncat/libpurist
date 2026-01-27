embed_resources(NotoSansFont1 
    fonts/noto-sans1/NotoSans-Regular.ttf
    fonts/noto-sans1/NotoSansHebrew-Regular.ttf
)

add_executable(apc_test
    apc_test.cpp
    ${NotoSansFont1}
)

target_include_directories(apc_test PUBLIC ${purist_third_party}/prefix/include/skia)
target_include_directories(apc_test PUBLIC ${purist_third_party}/prefix/include/skia/icu/source/common)

target_link_directories(apc_test PUBLIC ${purist_third_party}/prefix/lib-$<IF:$<CONFIG:Debug>,debug,release>)

target_link_libraries(apc_test PUBLIC 
    skia
#    webpmux
#    webpdemux
    webp
    jpeg
    png
    freetype2
    piex
#    GLX
#    fontconfig
#    z
    pthread
)
