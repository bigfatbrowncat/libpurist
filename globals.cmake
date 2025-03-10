# Globally adding libasan for debug builds
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)
add_compile_options($<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)
add_link_options($<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>)

# Making strict build checks everywhere
add_compile_options(-Wall -Wno-attributes -Werror)

# Disabling RTTI globally, because Skia is built without it
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

#set(LIB_DIR "lib-$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>")
