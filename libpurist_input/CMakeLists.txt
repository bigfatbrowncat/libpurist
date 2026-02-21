project(purist_input)

add_library(purist_input STATIC
    # Public includes
    include/purist/input/KeyboardHandler.h
    include/purist/input/KeyboardsProvider.h
    
    src/purist/input/Keyboard.cpp
    src/purist/input/Keyboard.h
    src/purist/input/Keyboards.cpp
    src/purist/input/Keyboards.h
)

find_package(PkgConfig REQUIRED)
pkg_search_module(EVDEV REQUIRED libevdev)
pkg_search_module(XKB_COMMON REQUIRED xkbcommon)

target_compile_features(purist_input PUBLIC cxx_std_17)
set_property(TARGET purist_input PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(purist_input PRIVATE ${EVDEV_INCLUDE_DIRS})
target_include_directories(purist_input PRIVATE ${XKB_COMMON_INCLUDE_DIRS})

target_include_directories(purist_input PUBLIC ${purist_third_party}/evdevw/include)

# Publishing the include dir
target_include_directories(purist_input PUBLIC ${purist_input_SOURCE_DIR}/include)

# Using includes from libpurist_platform
target_include_directories(purist_input PUBLIC ${purist_platform_SOURCE_DIR}/include)

target_link_directories(purist_input PUBLIC ${purist_third_party}/prefix/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-musl)
target_link_directories(purist_input PUBLIC ${purist_third_party}/prefix/lib)

target_link_libraries(purist_input PUBLIC 
    evdev
    xkbcommon
)

# Installing the library
install(TARGETS purist_input DESTINATION lib-$<IF:$<CONFIG:Debug>,debug,release>)
install(DIRECTORY include/ DESTINATION include)
