# Link the application with the required libraries (these are compiled independently within the subdirectories)
target_link_libraries(${PROJECT_NAME}
    fbw_io
    fbw_modes
    fbw_sys
)

# Set our include directories; this means we can include files based on their subdirectory paths and not have to use relative paths (../  = ew)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(io)
add_subdirectory(modes)
add_subdirectory(sys)
