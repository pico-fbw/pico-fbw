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
    DOC "Path to SimConnect dynamic library"
)
find_library(SimConnect_LIBRARY_STATIC SimConnect
    PATHS
        "$ENV{MSFS_SDK}\\SimConnect SDK\\lib\\static"
        "C:\\MSFS\\SDK\\SimConnect SDK\\lib\\static"
        "C:\\Program Files\\Microsoft Flight Simulator SDK\\SimConnect SDK\\lib\\static"
        "C:\\SimConnect SDK\\lib\\static"
    DOC "Path to SimConnect static library"
)

if (SimConnect_INCLUDE_DIR AND SimConnect_LIBRARY AND SimConnect_LIBRARY_STATIC)
    # Provide variables for consumers
    set(SimConnect_INCLUDE_DIRS ${SimConnect_INCLUDE_DIR})
    set(SimConnect_LIBRARIES ${SimConnect_LIBRARY})
    set(SimConnect_LIBRARIES_STATIC ${SimConnect_LIBRARY_STATIC})
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
    REQUIRED_VARS SimConnect_INCLUDE_DIR SimConnect_LIBRARY SimConnect_LIBRARY_STATIC
    VERSION_VAR SimConnect_VERSION
)
