cmake_minimum_required(VERSION 3.25)

# Globally adding libasan for debug build
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)
add_compile_options(-Wall -Werror)

add_link_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)


project(purist)

add_executable(purist 
    main.cpp

    Card.cpp
    Card.h
    Display.cpp
    Display.h
    Displays.cpp
    Displays.h
    DumbBuffer.cpp
    DumbBuffer.h
    DumbBufferMapping.cpp
    DumbBufferMapping.h
    FrameBuffer.cpp
    FrameBuffer.h
    ModeConnector.cpp
    ModeConnector.h
    ModeResources.cpp
    ModeResources.h
)

target_include_directories(purist PRIVATE /usr/include/libdrm)
target_link_libraries(purist drm gbm EGL GL)