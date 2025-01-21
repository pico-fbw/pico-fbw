# See platform/example/resources/example.cmake for comments regarding the structure of this file
add_compile_definitions(FBW_PLATFORM_HOST=1)
if (HOST_SKIP_WWW)
    add_compile_definitions(HOST_SKIP_WWW=1)
else()
    # littlefs generation parameters -- these don't actually matter as the host webserver doesn't use littlefs
    set(LFS_BLOCK_SIZE 1024)
    set(LFS_PROG_SIZE 1)
    set(LFS_IMG_SIZE 262144) # 256KB
endif()

function(setup_before_subdirs)
    if (CMAKE_HOST_WIN32)
        # If MSFS SimConnect SDK is installed, we can build extra features with it
        find_package(SimConnect)
        if (SimConnect_FOUND)
            add_compile_definitions(SIMCONNECT=1)
            # Pass to parent scope
            set(SimConnect_FOUND ${SimConnect_FOUND} PARENT_SCOPE)
            set(SimConnect_INCLUDE_DIRS ${SimConnect_INCLUDE_DIRS} PARENT_SCOPE)
            set(SimConnect_LIBRARIES ${SimConnect_LIBRARIES} PARENT_SCOPE)
        endif()
    endif()
    # Disable printf format warnings for host
    # This is because most host platforms are 64-bit and the format specifiers are meant for 32-bit,
    # so we would get a lot of warnings
    # Things still seem to work fine, so warnings are disabled
    add_compile_options(-Wno-format)
    add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.c)
endfunction()

function(setup_after_subdirs)
    # Link math library on Linux for trig functions
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        target_link_libraries(${PROJECT_NAME} m)
    endif()
endfunction()
