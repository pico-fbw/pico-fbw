add_subdirectory(${PLATFORM_NAME})
message("Target platform library is: '${PLATFORM_LIB}'")
# Link the target platform to the rest of the project
target_link_libraries(${PROJECT_NAME} ${PLATFORM_LIB})
# Platforms may optionally provide an include directory
if (EXISTS ${PLATFORM_PATH}/include)
    message("Using platform include directory: ${PLATFORM_PATH}/include")
    target_include_directories(${PLATFORM_LIB} PUBLIC ${PLATFORM_PATH}/include)
endif()
