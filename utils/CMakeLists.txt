cmake_minimum_required(VERSION 3.13)
project(mklittlefs)

set(CMAKE_CXX_STANDARD 11)

file(GLOB LITTLEFS_SRC "mklittlefs/littlefs/*.c")

add_executable(${PROJECT_NAME} mklittlefs/main.cpp ${LITTLEFS_SRC})
target_include_directories(
    ${PROJECT_NAME} PRIVATE
    mklittlefs
    mklittlefs/tclap
    mklittlefs/include
    mklittlefs/littlefs
)
target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
    "VERSION=\"0.2.3-62-g975bd0f\""
    "LITTLEFS_VERSION=\"v2.5.1\""
    "BUILD_CONFIG=\"\""
    "BUILD_CONFIG_NAME=\"pico-fbw internal\""
    "__NO_INLINE__"
    "LFS_NAME_MAX=255"
)
