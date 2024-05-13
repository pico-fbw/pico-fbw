set(FBW_FORMAT OFF CACHE BOOL "Format code with clang-format")

if (NOT FBW_FORMAT)
    message("Code will not be formatted (disabled)")
    return()
endif()

find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if (NOT CLANG_FORMAT_EXE)
    message(WARNING "FBW_FORMAT was selected but clang-format was not found")
    return()
endif()

message("clang-format found at ${CLANG_FORMAT_EXE}")
file(GLOB_RECURSE ALL_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/lib/fusion/*.c
    ${CMAKE_SOURCE_DIR}/lib/fusion/*.h
    ${CMAKE_SOURCE_DIR}/lib/fusion/drivers/*.c
    ${CMAKE_SOURCE_DIR}/lib/fusion/drivers/*.h
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
message("Code will be formatted")
add_dependencies(${PROJECT_NAME} clang-format)
