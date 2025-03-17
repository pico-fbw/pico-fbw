include(FetchContent)

set(EXTERNALLIBS_SOURCES "")
set(EXTERNALLIBS_INCLUDE_DIRS "")

message("Fetching littlefs")
FetchContent_Declare(
    littlefs
    GIT_REPOSITORY https://github.com/littlefs-project/littlefs
    GIT_TAG v2.10.1
)
FetchContent_MakeAvailable(littlefs)
list(APPEND EXTERNALLIBS_SOURCES
    ${littlefs_SOURCE_DIR}/lfs.c
    ${littlefs_SOURCE_DIR}/lfs_util.c
)
list(APPEND EXTERNALLIBS_INCLUDE_DIRS ${littlefs_SOURCE_DIR})

message("Fetching minmea")
FetchContent_Declare(
    minmea
    GIT_REPOSITORY https://github.com/kosma/minmea
    SOURCE_SUBDIR " " # Override the source directory with a nonexistant one so cmake doesn't try to run CMakeLists.txt
)
FetchContent_MakeAvailable(minmea)
list(APPEND EXTERNALLIBS_SOURCES
    ${minmea_SOURCE_DIR}/minmea.c
)
list(APPEND EXTERNALLIBS_INCLUDE_DIRS ${minmea_SOURCE_DIR})
# Most platforms don't support timegm, so fall back to mktime
target_compile_definitions(${PLATFORM_LIB} PUBLIC timegm=mktime)

message("Fetching parson")
FetchContent_Declare(
    parson
    GIT_REPOSITORY https://github.com/kgabis/parson
    SOURCE_SUBDIR " "
)
FetchContent_MakeAvailable(parson)
list(APPEND EXTERNALLIBS_SOURCES
    ${parson_SOURCE_DIR}/parson.c
)
list(APPEND EXTERNALLIBS_INCLUDE_DIRS ${parson_SOURCE_DIR})

message("Fetching semver")
FetchContent_Declare(
    semver
    GIT_REPOSITORY https://github.com/h2non/semver.c
    GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(semver)
list(APPEND EXTERNALLIBS_SOURCES
    ${semver_SOURCE_DIR}/semver.c
)
list(APPEND EXTERNALLIBS_INCLUDE_DIRS ${semver_SOURCE_DIR})

# Create a new include/wrapper directory to store the headers of the external libraries
# This is so that they can be included in the same way as the in-project libraries (#include "lib/____.h")
set(EXTERNALLIBS_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include")
set(EXTERNALLIBS_WRAPPER_LIB_DIR "${EXTERNALLIBS_INCLUDE_DIR}/lib")
file(MAKE_DIRECTORY ${EXTERNALLIBS_WRAPPER_LIB_DIR})
foreach(lib_dir IN LISTS EXTERNALLIBS_INCLUDE_DIRS)
    file(GLOB LIB_HEADERS "${lib_dir}/*.h")
    foreach(header IN LISTS LIB_HEADERS)
        # Copy the header file into the wrapper lib directory
        file(COPY ${header} DESTINATION ${EXTERNALLIBS_WRAPPER_LIB_DIR})
    endforeach()
endforeach()
