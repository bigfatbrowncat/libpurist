add_executable(blinking_screen
    blinking_screen.cpp
)
target_link_libraries(blinking_screen PUBLIC 
    purist_graphics
)
add_dependencies(blinking_screen purist_graphics)
