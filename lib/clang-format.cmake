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
    if (FBW_FORMAT)
        add_dependencies(${PROJECT_NAME} clang-format)
    endif()
else()
    message(WARNING "clang-format not found. Code will not be formatted.")
endif()
