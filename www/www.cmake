set(FBW_BUILD_WWW CACHE BOOL ON)
if (NOT $CACHE{FBW_BUILD_WWW})
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

# Define a custom www target that builds the web interface
add_custom_target(www
    COMMAND yarn install && yarn build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/www
    COMMENT "Building the web interface"
)

add_dependencies(${PROJECT_NAME} www)
