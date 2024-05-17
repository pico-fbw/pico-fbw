# Pull in ESP-IDF from the user-set environment variable
if (DEFINED ENV{IDF_PATH} AND (NOT IDF_PATH))
    set(IDF_PATH $ENV{IDF_PATH})
    message("Using IDF_PATH from environment ('${IDF_PATH}')")
endif()

set(IDF_PATH ${IDF_PATH} CACHE PATH "Path to ESP-IDF" FORCE)

# Validate the IDF_PATH
if (NOT IDF_PATH)
    message(
        FATAL_ERROR
        "IDF_PATH is not set. "
        "To build for \"${FBW_PLATFORM}\", you must download ESP-IDF and set the IDF_PATH environment variable to point to it. "
        "See https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html#get-started-get-esp-idf"
    )
endif()
if (NOT EXISTS ${IDF_PATH})
    message(FATAL_ERROR "Directory '${IDF_PATH}' does not exist")
endif()
if (NOT EXISTS ${IDF_PATH}/tools/cmake/idf.cmake)
    message(FATAL_ERROR "Directory '${IDF_PATH}' does not appear to contain ESP-IDF")
endif()

# For ESP-IDF builds to work, the `export` command (as part of ESP-IDF) must have been run in the current shell
# Check the PATH for the ESP-IDF tools (in this case xtensa-esp-elf-gcc compiler) and
# generate a warning to push the user in the right direction
find_program(TOOLS_IN_PATH xtensa-esp-elf-gcc PATH ENV{PATH})
if (NOT TOOLS_IN_PATH)
    set(EXPORT_COMMAND_NAME "export.sh")
    if (WIN32)
        set(EXPORT_COMMAND_NAME "export.bat")
    endif()
    message(
        WARNING
        "ESP-IDF tools were not found in your PATH. Proceed with caution. "
        "You probably didn't run ${EXPORT_COMMAND_NAME} from the ESP-IDF directory (${IDF_PATH}). "
        "If the configuration or build fails, run the script and try again."
    )
endif()

include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
