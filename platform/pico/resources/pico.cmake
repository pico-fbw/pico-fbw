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
    pico_add_extra_outputs(${PROJECT_NAME}) # Tell the pico-sdk to generate extra outputs, which includes .uf2 (easier to upload)
    # If the web interface is going to be built,
    # Select our custom linker script and assembly file that link the littlefs binary into the firmware
    if (${FBW_BUILD_WWW})
        target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/generated/www) # So lfs.S can find the binary
        target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/platform/pico/resources/lfs.S)
        pico_set_linker_script(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/platform/pico/resources/memmap.ld)
    endif()
    # FIXME: the custom linker script only works on debug builds? pico never boots on release builds
endfunction()
