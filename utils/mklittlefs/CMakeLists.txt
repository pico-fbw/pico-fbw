cmake_minimum_required(VERSION 3.13)
project(mklittlefs)

set(CMAKE_CXX_STANDARD 11)

file(GLOB LITTLEFS_SRC "littlefs/*.c")

add_executable(${PROJECT_NAME} main.cpp ${LITTLEFS_SRC})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "VERSION=\"0.2.3-75-g2f7654f\""
    "LITTLEFS_VERSION=\"v2.9.3\""
    "BUILD_CONFIG=\"\""
    "BUILD_CONFIG_NAME=\"pico-fbw internal\""
    "__NO_INLINE__"
    "LFS_NAME_MAX=255"
)
