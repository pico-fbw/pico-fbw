set(FBW_BUILD_WWW ON CACHE BOOL "Build the web interface")
if (NOT ${FBW_BUILD_WWW})
    message("Skipping web interface build")
    return()
endif()
message("Configuring web interface build")

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

# Define custom targets to build mklittlefs and the web interface

if (WIN32)
    set(MKLITTLEFS_EXE_NAME mklittlefs.exe)
else()
    set(MKLITTLEFS_EXE_NAME mklittlefs)
endif()
set(MKLITTLEFS_EXE ${CMAKE_BINARY_DIR}/mklittlefs/${MKLITTLEFS_EXE_NAME})
add_custom_target(mklittlefs
    COMMAND make
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${MKLITTLEFS_EXE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${MKLITTLEFS_EXE_NAME} ${CMAKE_BINARY_DIR}/${MKLITTLEFS_EXE_NAME}
    COMMAND ${CMAKE_COMMAND} -E remove ${MKLITTLEFS_EXE_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/utils/mklittlefs
    COMMENT "Building mklittlefs"
)

add_custom_target(www
    COMMAND yarn install && yarn build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/www
    COMMENT "Building the web interface"
)

add_custom_target(wwwfs
    COMMAND ${CMAKE_COMMAND} -E make_directory generated/www
    COMMAND ${MKLITTLEFS_EXE} -c www -b ${LFS_BLOCK_SIZE} -p ${LFS_PROG_SIZE} -s ${LFS_IMG_SIZE} generated/www/lfs.bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating littlefs image of web interface"
)

add_dependencies(${PROJECT_NAME} wwwfs)
add_dependencies(wwwfs www mklittlefs)
