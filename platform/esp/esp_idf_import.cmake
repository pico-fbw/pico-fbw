if (DEFINED ENV{IDF_PATH} AND (NOT IDF_PATH))
    set(IDF_PATH $ENV{IDF_PATH})
    message("Using IDF_PATH from environment ('${IDF_PATH}')")
endif ()

set(IDF_PATH ${IDF_PATH} CACHE PATH "Path to ESP-IDF" FORCE)

if (NOT EXISTS ${IDF_PATH})
    message(FATAL_ERROR "Directory '${IDF_PATH}' does not exist")
endif ()

if (NOT EXISTS ${IDF_PATH}/tools/cmake/idf.cmake)
    message(FATAL_ERROR "Directory '${IDF_PATH}' does not appear to contain ESP-IDF")
endif ()

include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

# TODO: finish esp32 implementation
# https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html#using-esp-idf-in-custom-cmake-projects

# have to run export.sh (maybe add a check for this in cmake?)
# cmake -B build -DFBW_PLATFORM=esp32 -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-esp32.cmake -GNinja
