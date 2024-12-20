cmake_minimum_required(VERSION 3.25)

# Globally adding libasan for debug builds
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)

# Making strict build checks everywhere
add_compile_options(-Wall -Werror)

# Disabling RTTI globally, because Skia is built without it
add_compile_options(-fno-rtti)


project(purist)

add_library(purist_graphics STATIC
    # Public includes
    include/purist/Platform.h
    include/purist/exceptions.h
    include/purist/graphics/interfaces.h

    # Private includes/sources
    src/purist/Platform.cpp

    src/purist/graphics/Card.cpp
    src/purist/graphics/Card.h
    src/purist/graphics/DisplayImpl.cpp
    src/purist/graphics/DisplayImpl.h
    src/purist/graphics/Displays.cpp
    src/purist/graphics/Displays.h
    src/purist/graphics/DumbBufferTargetSurfaceImpl.cpp
    src/purist/graphics/DumbBufferTargetSurfaceImpl.h
    src/purist/graphics/EGLTargetSurfaceImpl.cpp
    src/purist/graphics/EGLTargetSurfaceImpl.h
    src/purist/graphics/DumbBufferMapping.cpp
    src/purist/graphics/DumbBufferMapping.h
    src/purist/graphics/FrameBufferImpl.cpp
    src/purist/graphics/FrameBufferImpl.h
    src/purist/graphics/ModeConnector.cpp
    src/purist/graphics/ModeConnector.h
    src/purist/graphics/ModeCrtc.cpp
    src/purist/graphics/ModeCrtc.h
    src/purist/graphics/ModeEncoder.cpp
    src/purist/graphics/ModeEncoder.h
    src/purist/graphics/ModeResources.cpp
    src/purist/graphics/ModeResources.h
    src/purist/graphics/TargetSurfaceBackface.h
)

target_compile_features(purist_graphics PUBLIC cxx_std_17)
set_property(TARGET purist_graphics PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(purist_graphics PRIVATE /usr/include/libdrm)
target_include_directories(purist_graphics PUBLIC ${purist_SOURCE_DIR}/include)
target_link_libraries(purist_graphics PUBLIC 
    drm 
    gbm 
    EGL 
    GL
)

##########################################

add_library(purist_graphics_skia STATIC
    # Public includes
    include/purist/graphics/skia/SkiaEGLOverlay.h

    # Private includes/sources
    src/purist/graphics/skia/SkiaEGLOverlay.cpp
)

target_compile_features(purist_graphics_skia PUBLIC cxx_std_17)
set_property(TARGET purist_graphics_skia PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(purist_graphics_skia PUBLIC ${purist_SOURCE_DIR}/include)

target_include_directories(purist_graphics_skia PUBLIC ${purist_SOURCE_DIR}/third-party/prefix/include/skia)
target_link_directories(purist_graphics_skia PUBLIC ${purist_SOURCE_DIR}/third-party/prefix/lib-release)

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

##########################################

add_executable(blinking_screen
    examples/blinking_screen/blinking_screen.cpp
)
#target_include_directories(blinking_screen PUBLIC ${purist_SOURCE_DIR}/include)
#target_include_directories(blinking_screen PUBLIC ${purist_SOURCE_DIR}/third-party/prefix/include/skia)
#target_link_directories(blinking_screen PUBLIC ${purist_SOURCE_DIR}/third-party/prefix/lib-release)
target_link_libraries(blinking_screen PUBLIC 
    purist_graphics_skia
)
add_dependencies(blinking_screen purist_graphics_skia)