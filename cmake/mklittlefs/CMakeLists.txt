cmake_minimum_required(VERSION 3.13)

project(mklittlefs)
set(CMAKE_CXX_STANDARD 11)

execute_process(
    COMMAND git describe --tags
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE MKLITTLEFS_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git -C littlefs describe --tags
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE LITTLEFS_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

file(GLOB LITTLEFS_SRC "littlefs/*.c")
add_executable(${PROJECT_NAME} main.cpp ${LITTLEFS_SRC})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "VERSION=\"${MKLITTLEFS_VERSION}\""
    "LITTLEFS_VERSION=\"${LITTLEFS_VERSION}\""
    "BUILD_CONFIG=\"\""
    "BUILD_CONFIG_NAME=\"pico-fbw util\""
    "__NO_INLINE__"
    "LFS_NAME_MAX=255"
)
