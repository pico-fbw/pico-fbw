include(FetchContent)

# Create a common include/wrapper directory for external headers
set(EXTERNALLIBS_WRAPPER_LIB_DIR "${CMAKE_BINARY_DIR}/include/lib")
file(MAKE_DIRECTORY ${EXTERNALLIBS_WRAPPER_LIB_DIR})

# Imports a library into the project, creating a static library target and importing headers
function(import_library name dir)
    message(STATUS "Importing ${name}")
    # Get the source file(s) from the arguments
    set(sources "")
    foreach(src IN LISTS ARGN)
        list(APPEND sources "${dir}/${src}")
    endforeach()
    # Get the header files from the directory
    file(GLOB LIB_HEADERS "${dir}/*.h")
    # Add the library and set up includes
    add_library(${name} STATIC ${sources})
    target_include_directories(${name} PUBLIC ${dir})
    file(COPY ${LIB_HEADERS} DESTINATION ${EXTERNALLIBS_WRAPPER_LIB_DIR})
endfunction()

message("Fetching littlefs")
FetchContent_Declare(
    littlefs
    GIT_REPOSITORY https://github.com/littlefs-project/littlefs
    GIT_TAG v2.10.2
)
FetchContent_MakeAvailable(littlefs)
import_library(littlefs ${littlefs_SOURCE_DIR}
    lfs.c
    lfs_util.c
)

message("Fetching minmea")
FetchContent_Declare(
    minmea
    GIT_REPOSITORY https://github.com/kosma/minmea
    SOURCE_SUBDIR " " # Override the source directory with a nonexistant one so cmake doesn't try to run CMakeLists.txt
)
FetchContent_MakeAvailable(minmea)
import_library(minmea ${minmea_SOURCE_DIR}
    minmea.c
)
# Most platforms don't support timegm, so fall back to mktime when compiling minmea
# Also fix undefined uint type on some platforms
target_compile_definitions(minmea PRIVATE timegm=mktime)
if (FBW_PLATFORM STREQUAL "host")
    # Fix uint type on certain host platforms
    target_compile_definitions(minmea PRIVATE uint=uint32_t)
endif()

message("Fetching parson")
FetchContent_Declare(
    parson
    GIT_REPOSITORY https://github.com/kgabis/parson
    SOURCE_SUBDIR " "
)
FetchContent_MakeAvailable(parson)
import_library(parson ${parson_SOURCE_DIR}
    parson.c
)

message("Fetching semver")
FetchContent_Declare(
    semver
    GIT_REPOSITORY https://github.com/h2non/semver.c
    GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(semver)
import_library(semver ${semver_SOURCE_DIR}
    semver.c
)
