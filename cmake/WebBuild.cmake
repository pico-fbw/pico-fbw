# We need LFS_ variables to be defined in order to run mklittlefs later (and therefore to build the web interface)
include(CMakeDependentOption)
cmake_dependent_option(FBW_BUILD_WWW "Build the web interface" ON "DEFINED LFS_BLOCK_SIZE;DEFINED LFS_PROG_SIZE;DEFINED LFS_IMG_SIZE" OFF)
if (NOT FBW_BUILD_WWW)
    if (DEFINED LFS_BLOCK_SIZE AND DEFINED LFS_PROG_SIZE AND DEFINED LFS_IMG_SIZE)
        message("Web interface will NOT be built (disabled)")
    else()
        message("Web interface will NOT be built (unsupported)")
    endif()
    return()
endif()
add_compile_definitions(FBW_BUILD_WWW=1)
target_compile_definitions(${PLATFORM_LIB} PUBLIC FBW_BUILD_WWW=1)

# Ensure yarn is installed
find_package(yarn REQUIRED)

# Add mklittlefs as an external project so it will be built to be used later
include(ExternalProject)
set(MKLITTLEFS_DIR ${CMAKE_BINARY_DIR}/mklittlefs)
if (CMAKE_HOST_WIN32)
    set(MKLITTLEFS_EXE_NAME mklittlefs.exe)
else()
    set(MKLITTLEFS_EXE_NAME mklittlefs)
endif()
set(MKLITTLEFS_EXE ${MKLITTLEFS_DIR}/${MKLITTLEFS_EXE_NAME})
ExternalProject_Add(mklittlefs
    GIT_REPOSITORY https://github.com/earlephilhower/mklittlefs.git
        GIT_TAG 4.0.1
        GIT_SUBMODULES_RECURSE TRUE
    SOURCE_DIR ${MKLITTLEFS_DIR}
        BUILD_IN_SOURCE TRUE
    # mklittlefs is not a CMake project, so we need to copy a CMakeLists in
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cmake/mklittlefs/CMakeLists.txt <SOURCE_DIR>/CMakeLists.txt
    INSTALL_COMMAND ""
    COMMENT "Building mklittlefs"
)

# Add a target to build the web interface
# It depends on all files in the www directory, so it will only rebuild if any of those files change
file(GLOB_RECURSE WWW_FILES ${CMAKE_SOURCE_DIR}/www/*)
# To build the web interface, invoke the www.sh wrapper script which will respect nvm if installed
set(BUILD_WWW_CMD ${CMAKE_SOURCE_DIR}/www/www.sh ${CMAKE_SOURCE_DIR}/www ${YARN_EXE})
if (CMAKE_HOST_WIN32)
    # nvm doesn't exist on windows, so just attempt to invoke yarn directly
    set(BUILD_WWW_CMD ${YARN_EXE} install && ${YARN_EXE} build)
endif()
add_custom_command(
    # This command will also output an empty file whose modify timestamp can be used to check if/when the web interface has been built
    OUTPUT ${CMAKE_BINARY_DIR}/generated/www/built
    COMMAND ${BUILD_WWW_CMD}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated/www
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/generated/www/built
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/www
    DEPENDS ${WWW_FILES}
    USES_TERMINAL
    COMMENT "Building the web interface"
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(MKLITTLEFS_DEBUG_LEVEL 0)
else()
    set(MKLITTLEFS_DEBUG_LEVEL 1)
endif()
# This target uses the built mklittlefs binary and the built assets to create a littlefs image that will be included
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/www/lfs.bin
    # These LFS_ variables are defined by each platform in their respective .cmake files
    COMMAND ${MKLITTLEFS_EXE} -d ${MKLITTLEFS_DEBUG_LEVEL} -c www -b ${LFS_BLOCK_SIZE} -p ${LFS_PROG_SIZE} -s ${LFS_IMG_SIZE} generated/www/lfs.bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    # Similarly, this target depends on that file we created earlier, so if the web interface is rebuilt, this will be too
    DEPENDS ${CMAKE_BINARY_DIR}/generated/www/built mklittlefs
    USES_TERMINAL
    COMMENT "Creating littlefs image of web interface"
)

message("Web interface will be built")
add_custom_target(www DEPENDS ${CMAKE_BINARY_DIR}/generated/www/built)
add_custom_target(wwwfs DEPENDS ${CMAKE_BINARY_DIR}/generated/www/lfs.bin)
# Add the wwwfs target as a dependency of the platform target instead of the main ${PROJECT_NAME} target
# This way, lfs.bin is guaranteed to exist when the platform library is being compiled, which is what is most likely to need it
add_dependencies(${PLATFORM_LIB} wwwfs)

# Configure www-accessible version file
configure_file(${CMAKE_SOURCE_DIR}/www/src/helpers/version.ts.in ${CMAKE_SOURCE_DIR}/www/src/helpers/version.ts @ONLY)
