find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if (CLANG_FORMAT_EXE)
    message("clang-format found at ${CLANG_FORMAT_EXE}")
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/platform/*.c
        ${CMAKE_SOURCE_DIR}/platform/*.h
        ${CMAKE_SOURCE_DIR}/src/*.c
        ${CMAKE_SOURCE_DIR}/src/*.h
    )
    add_custom_target(
        clang-format
        COMMAND ${CLANG_FORMAT_EXE}
        -style=file
        -i
        ${ALL_SOURCE_FILES}
        COMMENT "Running clang-format on all source files"
    )
    set(FBW_FORMAT OFF CACHE BOOL "Format code with clang-format")
    if (${FBW_FORMAT})
        message("Code will be formatted")
        add_dependencies(${PROJECT_NAME} clang-format)
    else()
        message("Code will not be formatted (disabled)")
    endif()
else()
    message("Code will not be formatted (clang-format not found)")
endif()
