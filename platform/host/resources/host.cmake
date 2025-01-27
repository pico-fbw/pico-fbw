# See platform/example/resources/example.cmake for comments regarding the structure of this file
add_compile_definitions(FBW_PLATFORM_HOST=1)

if (HOST_SKIP_WWW)
    add_compile_definitions(HOST_SKIP_WWW=1)
else()
    set(LFS_BLOCK_SIZE 4096)
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
    if (NOT HOST_SKIP_WWW)
        # Move the generated lfs.bin into pico-fbw's emulated filesystem directory
        if (CMAKE_HOST_WIN32)
            set(DEST_DIR "$ENV{APPDATA}/.pico-fbw")
        else()
            set(DEST_DIR "$ENV{HOME}/.pico-fbw")
        endif()
        add_custom_command(
            TARGET wwwfs
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/generated/www/lfs.bin ${DEST_DIR}/wwwfs.bin
            COMMENT "Copying lfs.bin to pico-fbw directory"
        )
    endif()
endfunction()
