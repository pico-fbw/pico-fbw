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

# The below functions will be called by the main CMakeLists.txt file later, to further to set up the project for buulding

# Will run after the project has been initialized, but before the subdirectories are added and processed
function(setup_before_subdirs)
    pico_sdk_init()
    add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.c)
endfunction()

# Will run after the subdirectories have been added and processed
function(setup_after_subdirs)
    pico_add_extra_outputs(${PROJECT_NAME}) # Tell the pico-sdk to generate extra outputs, which includes .uf2 (easier to upload)
endfunction()
