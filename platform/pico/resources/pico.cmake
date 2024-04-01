# Define a macro FBW_PLATFORM_PICO that can be used in the code to determine the current platform
add_definitions(-DFBW_PLATFORM_PICO)

# Import the pico-sdk
include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
# Set up some CMake cross-compilation variables
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_OSX_ARCHITECTURES "")
# Forward the user-set platform (normal Pico or W) to the pico-sdk
set(PICO_BOARD ${FBW_PLATFORM})
# The pico-sdk does require some assembly and C++ features so we need to add them to the project
project(${PROJECT_NAME} LANGUAGES ASM CXX)
set(CMAKE_CXX_STANDARD 17)
# Define littlefs filesystem parameters that will be used by mklittlefs to generate the filesystem
set(LFS_BLOCK_SIZE 4096)
set(LFS_PROG_SIZE 256)
set(LFS_IMG_SIZE 262144) # 256KB

# The below functions will be called by the main CMakeLists.txt file later, to further to set up the project for buulding

# Will run after the project has been initialized, but before the subdirectories are added and processed
function(setup_before_subdirs)
    pico_sdk_init()
    add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.c)
endfunction()

# Will run after the subdirectories have been added and processed
function(setup_after_subdirs)
    # Both USB and UART (pins moved) are enabled by default (either can be disabled by setting to 0, if needed)
    # These are compiled into stdio, so using printf() will output to whatever is enabled
    pico_enable_stdio_usb(${PROJECT_NAME} 1)
    pico_enable_stdio_uart(${PROJECT_NAME} 1)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_DEFAULT_UART_TX_PIN=12 # Must be a pin with UART0 TX
        PICO_DEFAULT_UART_RX_PIN=13 # Must be a pin with UART0 RX
    )
    pico_add_extra_outputs(${PROJECT_NAME}) # Tell the pico-sdk to generate extra outputs, which includes .uf2 (easier to upload)
    # If the web interface is going to be built,
    if (${FBW_BUILD_WWW})
        # Compile our custom assembly file that includes the littlefs binary data into the final executable
        target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/generated/www) # So lfs.S can find the binary data
        target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/platform/pico/resources/lfs.S)
        # If any of the files in the www directory have been changed, we "touch" the lfs.S file to force it to be recompiled
        # This means that if the web interface is changed, linking will be re-run and the new files will be included
        file(GLOB_RECURSE WWW_FILES ${CMAKE_SOURCE_DIR}/www/*)
        add_custom_command(
            OUTPUT ${CMAKE_SOURCE_DIR}/platform/pico/resources/lfs.S
            COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_SOURCE_DIR}/platform/pico/resources/lfs.S
            DEPENDS ${WWW_FILES}
        )
    endif()
    # Always use our custom linker script regardless of the web interface, this is so littlefs can always be in the same place
    pico_set_linker_script(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/platform/pico/resources/memmap.ld)
endfunction()
