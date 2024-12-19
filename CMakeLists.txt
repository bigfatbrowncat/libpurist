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
    include/purist/platform/Platform.h
    include/purist/platform/exceptions.h
    include/purist/platform/interfaces.h

    # Private includes/sources
    src/purist/platform/Card.cpp
    src/purist/platform/Card.h
    src/purist/platform/DisplayImpl.cpp
    src/purist/platform/DisplayImpl.h
    src/purist/platform/Displays.cpp
    src/purist/platform/Displays.h
    src/purist/platform/DumbBufferTargetSurfaceImpl.cpp
    src/purist/platform/DumbBufferTargetSurfaceImpl.h
    src/purist/platform/EGLTargetSurfaceImpl.cpp
    src/purist/platform/EGLTargetSurfaceImpl.h
    src/purist/platform/Platform.cpp
    src/purist/platform/DumbBufferMapping.cpp
    src/purist/platform/DumbBufferMapping.h
    src/purist/platform/FrameBufferImpl.cpp
    src/purist/platform/FrameBufferImpl.h
    src/purist/platform/ModeConnector.cpp
    src/purist/platform/ModeConnector.h
    src/purist/platform/ModeCrtc.cpp
    src/purist/platform/ModeCrtc.h
    src/purist/platform/ModeEncoder.cpp
    src/purist/platform/ModeEncoder.h
    src/purist/platform/ModeResources.cpp
    src/purist/platform/ModeResources.h
    src/purist/platform/TargetSurfaceBackface.h
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