# See platform/example/resources/example.cmake for comments regarding the structure of this file
add_compile_definitions(FBW_PLATFORM_HOST=1)

set(LFS_BLOCK_SIZE 4096)
set(LFS_PROG_SIZE 1)
set(LFS_IMG_SIZE 262144) # 256KB

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
    if (${FBW_BUILD_WWW})
        # Fetch mongoose as a dependency ("wifi" support requires it)
        include(Mongoose)
        target_sources(platform_host PRIVATE ${mongoose_SOURCE_DIR}/mongoose.c)
        target_include_directories(platform_host PRIVATE ${mongoose_SOURCE_DIR})
        include_wwwfs(${PLATFORM_PATH}/flash.c)
    endif()
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        # Link math and time libraries
        target_link_libraries(${PROJECT_NAME} m rt)
    endif()
endfunction()
