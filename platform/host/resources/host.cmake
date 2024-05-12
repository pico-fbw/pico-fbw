# See platform/example/resources/example.cmake for comments regarding the structure of this file
add_definitions(-DFBW_PLATFORM_HOST)

function(setup_before_subdirs)
    add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.c)
endfunction()

function(setup_after_subdirs)
    # Link math library on Linux for trig functions
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        target_link_libraries(fbw_lib m)
    endif()
endfunction()
