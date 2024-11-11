# Try to find Yarn using nvm if it's being used
if (EXISTS "$ENV{NVM_DIR}/nvm.sh")
    message(STATUS "nvm detected, using it to find yarn")
    execute_process(
        COMMAND bash -c "source $ENV{NVM_DIR}/nvm.sh && which yarn"
        OUTPUT_VARIABLE YARN_EXE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    # Not using nvm, leave it to cmake to find yarn
    find_program(YARN_EXE yarn) # Fall back to standard find_program
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(yarn
    REQUIRED_VARS YARN_EXE
    REASON_FAILURE_MESSAGE "yarn was not found, but is required to build the web interface!"
    FAIL_MESSAGE "Please install yarn at: https://yarnpkg.com/getting-started/install."
)
