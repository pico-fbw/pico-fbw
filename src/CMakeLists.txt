# Define the entry point for the application
add_executable(${PROJECT_NAME} main.c)

# Link the application with the required libraries (these are compiled independently within the subdirectories)
target_link_libraries(${PROJECT_NAME}
    fbw_io
    fbw_lib
    fbw_modes
    fbw_sys
    fbw_wifly
)
# Set our include directories; this means we can include files based on their subdirectory paths and not have to use relative paths (../  = ew)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)

# Run platform-specific commands
if (FBW_PLATFORM STREQUAL "pico" OR FBW_PLATFORM STREQUAL "pico_w")
    pico_add_extra_outputs(${PROJECT_NAME}) # Tell the pico-sdk to generate extra outputs, which includes .uf2 (preferred)
    # Link some Pico W-specific libraries 
    if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
        target_link_libraries(${PROJECT_NAME} pico_cyw43_arch_none)
        include_directories(${CMAKE_CURRENT_LIST_DIR}/wifly)
    endif ()
endif()

add_subdirectory(io)
add_subdirectory(modes)
add_subdirectory(sys)
# Wi-Fly is always enabled in some capacity (to allow for the API to interact with it regardless of Pico W).
# LWIP and the whole HTTP stack only gets compiled if the Pico W is detected (this is all done in the Wi-Fly subdirectory).
add_subdirectory(wifly)
