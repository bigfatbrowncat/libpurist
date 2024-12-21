project(purist_graphics_skia)

add_library(purist_graphics_skia STATIC
    # Public includes
    include/purist/graphics/skia/SkiaEGLOverlay.h

    # Private includes/sources
    src/purist/graphics/skia/SkiaEGLOverlay.cpp
)

target_compile_features(purist_graphics_skia PUBLIC cxx_std_17)
set_property(TARGET purist_graphics_skia PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(purist_graphics_skia PUBLIC ${purist_graphics_skia_SOURCE_DIR}/include)

target_include_directories(purist_graphics_skia PUBLIC ${purist_third_party}/prefix/include/skia)
target_link_directories(purist_graphics_skia PUBLIC ${purist_third_party}/prefix/lib-release)

target_link_libraries(purist_graphics_skia PUBLIC 
    purist_graphics

    skia
    webpmux
    webpdemux
    webp
    jpeg
    png
    freetype
    GLX
    fontconfig
    z
)
