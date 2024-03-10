# Make sure you change the name of this file from "example.cmake" to the name of the directory that contains your platform!
# For example, if your platform/directory was called "lipsum", you would rename this file to "lipsum.cmake".

# Change this to the name of your platform.
# This will be used within the code to determine which platform is being used (see defs.h for more information).
# Although it isn't required to be, we strongly recommend that you use the same name as the directory name you choose (but in all caps).
# For example, if your directory is called "lipsum", you should set this to "-DFBW_PLATFORM_LIPSUM".
add_definitions(-DFBW_PLATFORM_EXAMPLE)

# This function will be called before any subdirectories are added (including the platform directory).
# Take a look at other platforms to see how this is typically used, but some ideas include:
# Initializing any SDKs or libraries you may be using, defining the main executable, etc.
function(setup_before_subdirs)
    # The following line is very common and required for most platforms, but feel free to remove it if it doesn't apply to your platform.
    add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.c)
endfunction()

# This function will be called after all subdirectories have been added.
# Use this to run any last-minute setup that needs to be done after all subdirectories have been added,
# such as further platform/SDK setup, linking, etc.
function(setup_after_subdirs)
    
endfunction()
