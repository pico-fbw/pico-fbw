find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if (CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.c ${PROJECT_SOURCE_DIR}/*.h)
    file(GLOB_RECURSE LIB_FILES ${PROJECT_SOURCE_DIR}/lib/*.c ${PROJECT_SOURCE_DIR}/lib/*.h)
    list(REMOVE_ITEM ALL_SOURCE_FILES ${LIB_FILES})
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
        message("Code will be formatted (clang-format)")
        add_dependencies(${PROJECT_NAME} clang-format)
    else()
        message("Code will not be formatted (disabled)")
    endif()
else()
    message("Code will not be formatted (clang-format not found)")
endif()
