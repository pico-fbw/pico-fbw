set(FBW_BUILD_WWW ON CACHE BOOL "Build the web interface")
if (NOT FBW_BUILD_WWW)
    message("Skipping web interface build")
    return()
endif()
message("Configuring web interface build")

# Ensure that LFS_ variables are defined
if (NOT DEFINED LFS_BLOCK_SIZE OR NOT DEFINED LFS_PROG_SIZE OR NOT DEFINED LFS_IMG_SIZE)
    message(WARNING "littlefs configuration not defined, skipping web interface build")
    set(FBW_BUILD_WWW OFF CACHE BOOL "Build the web interface" FORCE)
    return()
endif()

# Check to ensure npm and yarn are installed
find_program(NPM_EXECUTABLE npm)
if (NPM_EXECUTABLE)
    message("npm found at ${NPM_EXECUTABLE}")
endif()
find_program(YARN_EXECUTABLE yarn)
if (YARN_EXECUTABLE)
    message("yarn found at ${YARN_EXECUTABLE}")
endif()

if (NOT NPM_EXECUTABLE)
    message(FATAL_ERROR "npm was not found, but is required to build the web interface!
    npm can be installed from https://docs.npmjs.com/downloading-and-installing-node-js-and-npm.")
endif()
if (NOT YARN_EXECUTABLE)
    # No yarn but we do have npm which can be used to install install yarn
    message("yarn not found, trying to install now...")
    execute_process(COMMAND ${NPM_EXECUTABLE} install -g yarn)
    find_program(YARN_EXECUTABLE yarn)
    if (NOT YARN_EXECUTABLE)
        message(FATAL_ERROR "yarn could not be installed!")
    endif()
endif()

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
        GIT_TAG 4.0.0
        GIT_SUBMODULES_RECURSE TRUE
    SOURCE_DIR ${MKLITTLEFS_DIR}
        BUILD_IN_SOURCE TRUE
    # mklittlefs is not a CMake project, so we need to copy one in
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/utils/mklittlefs/CMakeLists.txt <SOURCE_DIR>/CMakeLists.txt
    INSTALL_COMMAND ""
    COMMENT "Building mklittlefs"
)

# Add a target to build the web interface
# It depends on all files in the www directory, so it will only rebuild if any of those files change
file(GLOB_RECURSE WWW_FILES ${CMAKE_SOURCE_DIR}/www/*)
add_custom_command(
    # This command will output an empty file that can be used to check if the web interface has been built
    OUTPUT ${CMAKE_BINARY_DIR}/generated/www/built
    COMMAND yarn install && yarn build
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/generated/www/built
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/www
    DEPENDS ${WWW_FILES}
    USES_TERMINAL
    COMMENT "Building the web interface"
)
# This target uses the built mklittlefs binary and the built assets to create a littlefs image that will be included
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/www/lfs.bin
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated/www
    # These LFS_ variables are defined by each platform in their respective .cmake files
    COMMAND ${MKLITTLEFS_EXE} -d 1 -c www -b ${LFS_BLOCK_SIZE} -p ${LFS_PROG_SIZE} -s ${LFS_IMG_SIZE} generated/www/lfs.bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    # Similarly, this target depends on that file we created earlier, so if the web interface is rebuilt, this will be too
    DEPENDS ${CMAKE_BINARY_DIR}/generated/www/built
    USES_TERMINAL
    COMMENT "Creating littlefs image of web interface"
)

add_custom_target(www ALL DEPENDS ${CMAKE_BINARY_DIR}/generated/www/built)
add_custom_target(wwwfs ALL DEPENDS ${CMAKE_BINARY_DIR}/generated/www/lfs.bin)
add_dependencies(${PROJECT_NAME} wwwfs)
