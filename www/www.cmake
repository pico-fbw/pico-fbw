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
if (EXISTS "$ENV{NVM_DIR}/nvm.sh")
    message("nvm detected, using it to find npm and yarn")
    set(USING_NVM ON)
    execute_process(
        COMMAND bash -c "source $ENV{NVM_DIR}/nvm.sh && which npm"
        OUTPUT_VARIABLE NPM_EXE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND bash -c "source $ENV{NVM_DIR}/nvm.sh && which yarn"
        OUTPUT_VARIABLE YARN_EXE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    # Not using nvm, leave it to cmake to find npm and yarn
    find_program(NPM_EXE npm)
    find_program(YARN_EXE yarn)
endif()

if (NPM_EXE)
    message("npm found at ${NPM_EXE}")
else()
    message(FATAL_ERROR "npm was not found, but is required to build the web interface!
    npm can be installed from https://docs.npmjs.com/downloading-and-installing-node-js-and-npm.")
endif()
if (YARN_EXE)
    message("yarn found at ${YARN_EXE}")
else()
    message(FATAL_ERROR "yarn was not found, but is required to build the web interface!
    yarn can be installed from https://yarnpkg.com/getting-started/install.")
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
        GIT_TAG 4.0.1
        GIT_SUBMODULES_RECURSE TRUE
    SOURCE_DIR ${MKLITTLEFS_DIR}
        BUILD_IN_SOURCE TRUE
    # mklittlefs is not a CMake project, so we need to copy one in
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/utils/mklittlefs/CMakeLists.txt <SOURCE_DIR>/CMakeLists.txt
    INSTALL_COMMAND ""
    COMMENT "Building mklittlefs"
)

# Add a target to build the web interface
# It depends on all files in the www directory, so it will only rebuild if any of those files change
file(GLOB_RECURSE WWW_FILES ${PROJECT_SOURCE_DIR}/www/*)
if (NOT CMAKE_HOST_WIN32)
    # Invoke our custom wrapper script to ensure that nvm is sourced
    add_custom_command(
        # This command will also output an empty file whose modify timestamp can be used to check if/when the web interface has been built
        OUTPUT ${CMAKE_BINARY_DIR}/generated/www/built
        COMMAND ${PROJECT_SOURCE_DIR}/www/www.sh ${PROJECT_SOURCE_DIR}/www ${YARN_EXE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated/www
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/generated/www/built
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/www
        DEPENDS ${WWW_FILES}
        USES_TERMINAL
        COMMENT "Building the web interface"
    )
else()
    # Because cmake is weird, no, there is (probably) not a better way than copying this function twice...sigh
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/generated/www/built
        COMMAND ${YARN_EXE} install && ${YARN_EXE} build # nvm doesn't exist on windows so just attempt to run directly
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated/www
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/generated/www/built
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/www
        DEPENDS ${WWW_FILES}
        USES_TERMINAL
        COMMENT "Building the web interface"
    )
endif()

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

add_custom_target(www DEPENDS ${CMAKE_BINARY_DIR}/generated/www/built)
add_custom_target(wwwfs DEPENDS ${CMAKE_BINARY_DIR}/generated/www/lfs.bin)
add_dependencies(${PROJECT_NAME} wwwfs)
