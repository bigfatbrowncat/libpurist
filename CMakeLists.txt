cmake_minimum_required(VERSION 3.25)

# Globally adding libasan for debug build
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)
add_compile_options(-Wall -Werror)

add_link_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)

project(purist)

add_library(purist STATIC
    include/purist/platform/Platform.h
    include/purist/platform/exceptions.h
    include/purist/platform/interfaces.h

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

target_compile_features(purist PUBLIC cxx_std_17)
target_include_directories(purist PUBLIC ${purist_SOURCE_DIR}/include)
target_include_directories(purist PRIVATE /usr/include/libdrm)
target_link_libraries(purist PUBLIC drm gbm EGL GL)

##########################################

add_executable(blinking_screen
    examples/blinking_screen/blinking_screen.cpp
)
target_include_directories(blinking_screen PUBLIC ${purist_SOURCE_DIR}/include)
target_link_libraries(blinking_screen purist)
add_dependencies(blinking_screen purist)