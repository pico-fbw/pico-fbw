# Try to find pnpm using nvm if it's being used
if (EXISTS "$ENV{NVM_DIR}/nvm.sh")
    message("nvm detected, using it to find pnpm")
    execute_process(
        COMMAND bash -c "source $ENV{NVM_DIR}/nvm.sh && which pnpm"
        OUTPUT_VARIABLE PNPM_EXE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    # Not using nvm, leave it to cmake to find pnpm
    find_program(PNPM_EXE pnpm) # Fall back to standard find_program
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pnpm
    REQUIRED_VARS PNPM_EXE
    REASON_FAILURE_MESSAGE "pnpm was not found, but is required to build the web interface!"
    FAIL_MESSAGE "Please install pnpm via https://pnpm.io/installation."
)
