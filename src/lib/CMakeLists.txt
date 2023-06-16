add_library(fbw_lib
    pid.c
    pidtune.c
)

target_link_libraries(fbw_lib
    pico_stdlib
)    
