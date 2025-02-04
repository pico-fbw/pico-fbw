# Set our include directories
# This means we can include files based on their subdirectory paths and not have to use relative paths 
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(ctrl)
add_subdirectory(io)
add_subdirectory(modes)
add_subdirectory(sys)
