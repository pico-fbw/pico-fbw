find_path(SimConnect_INCLUDE_DIR SimConnect.h
    PATHS
        "$ENV{MSFS_SDK}\\SimConnect SDK\\include" # Default MSFS SDK path
        # Typical SDK paths
        "C:\\MSFS\\SDK\\SimConnect SDK\\include"
        "C:\\Program Files\\Microsoft Flight Simulator SDK\\SimConnect SDK\\include"
        "C:\\SimConnect SDK\\include"
    DOC "Path to SimConnect.h"
)

find_library(SimConnect_LIBRARY SimConnect
    PATHS
        "$ENV{MSFS_SDK}\\SimConnect SDK\\lib"
        "C:\\MSFS\\SDK\\SimConnect SDK\\lib"
        "C:\\Program Files\\Microsoft Flight Simulator SDK\\SimConnect SDK\\lib"
        "C:\\SimConnect SDK\\lib"
    DOC "Path to SimConnect library"
)

if (SimConnect_INCLUDE_DIR AND SimConnect_LIBRARY)
    # Provide variables for consumers
    set(SimConnect_INCLUDE_DIRS ${SimConnect_INCLUDE_DIR})
    set(SimConnect_LIBRARIES ${SimConnect_LIBRARY})
    # Get version from version.txt
    get_filename_component(SimConnect_BASE_DIR ${SimConnect_INCLUDE_DIR} DIRECTORY)
    get_filename_component(SimConnect_SDK_ROOT ${SimConnect_BASE_DIR} DIRECTORY)
    set(SimConnect_VERSION_FILE "${SimConnect_SDK_ROOT}/version.txt")
    if (EXISTS ${SimConnect_VERSION_FILE})
        file(READ ${SimConnect_VERSION_FILE} SimConnect_VERSION_CONTENTS)
        string(STRIP "${SimConnect_VERSION_CONTENTS}" SimConnect_VERSION)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SimConnect
    REQUIRED_VARS SimConnect_INCLUDE_DIR SimConnect_LIBRARY
    VERSION_VAR SimConnect_VERSION
)
