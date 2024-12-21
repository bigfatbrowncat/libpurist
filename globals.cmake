# Globally adding libasan for debug builds
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_compile_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
add_link_options($<$<CONFIG:DEBUG>:-fsanitize-recover=all>)

# Making strict build checks everywhere
add_compile_options(-Wall -Werror)

# Disabling RTTI globally, because Skia is built without it
add_compile_options(-fno-rtti)
