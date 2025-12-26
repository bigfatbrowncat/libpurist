# Globally adding libasan for debug builds
#add_compile_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize=undefined>)
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)
add_compile_options($<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>)
#add_link_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize=undefined>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)
add_link_options($<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>)

# Making strict build checks everywhere
#add_compile_options(-Wall -Wno-attributes -Werror)
#add_link_options(-v)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disabling RTTI globally, because Skia is built without it
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

add_compile_definitions($<$<CONFIG:DEBUG>:-DSK_TRIVIAL_ABI=[[clang::trivial_abi]]>)

#add_compile_definitions(-DSK_DEBUG)

#set(LIB_DIR "lib-$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>")

# Configure pkg-config for sysroot
set(ENV{PKG_CONFIG_DIR} "")
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
