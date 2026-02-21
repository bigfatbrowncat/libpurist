project(purist_platform)

add_library(purist_platform STATIC
    # Public includes
    include/purist/Platform.h
    include/purist/exceptions.h

    # Private includes/sources
    src/purist/Platform.cpp
    src/purist/global_init.cpp
    src/purist/global_init.h
)

find_package(PkgConfig REQUIRED)
# #pkg_search_module(EVDEV REQUIRED libevdev)
# pkg_search_module(DRM REQUIRED libdrm)
# pkg_search_module(GBM REQUIRED gbm)
# #pkg_search_module(XKB_COMMON REQUIRED xkbcommon)
# pkg_search_module(GLES2 REQUIRED glesv2)
# pkg_search_module(GLVND REQUIRED libglvnd)

target_compile_features(purist_platform PUBLIC cxx_std_17)
set_property(TARGET purist_platform PROPERTY POSITION_INDEPENDENT_CODE ON)
# target_include_directories(purist_graphics PRIVATE ${DRM_INCLUDE_DIRS})
# target_include_directories(purist_graphics PRIVATE ${GBM_INCLUDE_DIRS})
# #target_include_directories(purist_graphics PRIVATE ${EVDEV_INCLUDE_DIRS})
# #target_include_directories(purist_graphics PRIVATE ${XKB_COMMON_INCLUDE_DIRS})
# target_include_directories(purist_graphics PUBLIC ${GLES2_INCLUDE_DIRS})

#target_include_directories(purist_graphics PUBLIC ${purist_third_party}/evdevw/include)
target_include_directories(purist_platform PUBLIC ${purist_platform_SOURCE_DIR}/include)

target_link_directories(purist_platform PUBLIC ${purist_third_party}/prefix/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-musl)
target_link_directories(purist_platform PUBLIC ${purist_third_party}/prefix/lib)

target_link_libraries(purist_platform PUBLIC 
)

# Installing the library
install(TARGETS purist_platform DESTINATION lib-$<IF:$<CONFIG:Debug>,debug,release>)
install(DIRECTORY include/ DESTINATION include)
