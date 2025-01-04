project(purist_graphics_skia)

add_library(purist_graphics_skia STATIC
    # Public includes
    include/purist/graphics/skia/DisplayContentsSkia.h
    include/purist/graphics/skia/SkiaOverlay.h
    include/purist/graphics/skia/TextInput.h

    # Private includes/sources
    src/purist/graphics/skia/DisplayContentsSkia.cpp
    src/purist/graphics/skia/SkiaOverlay.cpp
    src/purist/graphics/skia/SkiaRasterOverlay.cpp
    src/purist/graphics/skia/SkiaRasterOverlay.h
    src/purist/graphics/skia/SkiaEGLOverlay.cpp
    src/purist/graphics/skia/SkiaEGLOverlay.h
    src/purist/graphics/skia/TextInput.cpp
)

target_compile_features(purist_graphics_skia PUBLIC cxx_std_17)
set_property(TARGET purist_graphics_skia PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(purist_graphics_skia PUBLIC ${purist_graphics_skia_SOURCE_DIR}/include)

target_include_directories(purist_graphics_skia PUBLIC ${purist_third_party}/prefix/include/skia)
target_include_directories(purist_graphics_skia PUBLIC ${purist_third_party}/prefix/include/skia/icu/source/common)

target_link_directories(purist_graphics_skia PUBLIC ${purist_third_party}/prefix/lib-$<IF:$<CONFIG:Debug>,debug,release>)

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

# Installing the library
install(TARGETS purist_graphics_skia DESTINATION lib-$<IF:$<CONFIG:Debug>,debug,release>)

# Resolving Skia files
file (GLOB SKIA_LIBS_DEBUG ${purist_third_party}/prefix/lib-debug/*.a)
file (GLOB SKIA_LIBS_RELEASE ${purist_third_party}/prefix/lib-release/*.a)
file (GLOB_RECURSE SKIA_INCLUDES ${purist_third_party}/prefix/include/**/*.h)

set(_files ${SKIA_LIBS_DEBUG} ${SKIA_LIBS_RELEASE} ${SKIA_INCLUDES})

foreach (_file ${_files})
    get_filename_component(_resolvedFile "${_file}" REALPATH)
    file(RELATIVE_PATH _resolvedRelFile ${purist_third_party}/prefix ${_file})
    get_filename_component(_resolvedDirName ${_resolvedRelFile} DIRECTORY)
    install(FILES ${_resolvedFile} DESTINATION ${_resolvedDirName})
endforeach()
