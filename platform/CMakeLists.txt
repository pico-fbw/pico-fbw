add_subdirectory(${PLATFORM_DIR})
message("Target platform library is: platform_${PLATFORM_DIR}")
# Link the target platform to the rest of the project
target_link_libraries(${PROJECT_NAME} platform_${PLATFORM_DIR})
# Platforms may optionally provide an include directory
if (EXISTS ${PLATFORM_PATH}/include)
    message("Using platform include directory: ${PLATFORM_PATH}/include")
    target_include_directories(platform_${PLATFORM_DIR} PUBLIC ${PLATFORM_PATH}/include)
endif()
