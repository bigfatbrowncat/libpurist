project(purist_graphics)

add_library(purist_graphics STATIC
    # Public includes
    include/purist/Platform.h
    include/purist/exceptions.h
    include/purist/graphics/Display.h
    include/purist/graphics/DisplayContentsHandler.h
    include/purist/graphics/FrameBuffer.h
    include/purist/graphics/Mode.h
    include/purist/graphics/TargetSurface.h

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
    src/purist/graphics/ModeModeInfo.cpp
    src/purist/graphics/ModeModeInfo.h
    src/purist/graphics/ModeCrtc.cpp
    src/purist/graphics/ModeCrtc.h
    src/purist/graphics/ModeEncoder.cpp
    src/purist/graphics/ModeEncoder.h
    src/purist/graphics/ModeResources.cpp
    src/purist/graphics/ModeResources.h
    src/purist/graphics/TargetSurfaceBackface.h
    
    src/purist/input/Keyboard.cpp
    src/purist/input/Keyboard.h
    src/purist/input/Keyboards.cpp
    src/purist/input/Keyboards.h
)

find_package(PkgConfig REQUIRED)
pkg_search_module(EVDEV REQUIRED libevdev)
pkg_search_module(DRM REQUIRED libdrm)
pkg_search_module(GBM REQUIRED gbm)
pkg_search_module(XKB_COMMON REQUIRED xkbcommon)
pkg_search_module(GLES2 REQUIRED glesv2)
pkg_search_module(GLVND REQUIRED libglvnd)

target_compile_features(purist_graphics PUBLIC cxx_std_17)
set_property(TARGET purist_graphics PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(purist_graphics PRIVATE ${DRM_INCLUDE_DIRS})
target_include_directories(purist_graphics PRIVATE ${GBM_INCLUDE_DIRS})
target_include_directories(purist_graphics PRIVATE ${EVDEV_INCLUDE_DIRS})
target_include_directories(purist_graphics PRIVATE ${XKB_COMMON_INCLUDE_DIRS})
target_include_directories(purist_graphics PUBLIC ${GLES2_INCLUDE_DIRS})

target_include_directories(purist_graphics PUBLIC ${purist_third_party}/evdevw/include)
target_include_directories(purist_graphics PUBLIC ${purist_graphics_SOURCE_DIR}/include)

target_link_directories(purist_graphics PUBLIC ${purist_third_party}/prefix/lib/aarch64-linux-musl)

target_link_libraries(purist_graphics PUBLIC 
    evdev
    xkbcommon

    drm 
    gbm 
    EGL         # from glvnd
    GLdispatch  # from glvnd
    GLESv2      # from glvnd
)
#    OpenGL

# Installing the library
install(TARGETS purist_graphics DESTINATION lib-$<IF:$<CONFIG:Debug>,debug,release>)
install(DIRECTORY include/ DESTINATION include)
