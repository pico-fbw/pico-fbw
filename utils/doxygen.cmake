set(FBW_DOCS OFF CACHE BOOL "Generate docs using Doxygen")

if (NOT FBW_DOCS)
    message("Documentation will not be generated (disabled)")
    return()
endif()

find_package(Doxygen REQUIRED)
if (NOT DOXYGEN_FOUND)
    message(WARNING "FBW_DOCS was selected but Doxygen was not found")
    return()
endif()

set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR})
set(DOXYGEN_OUTPUT_DIR ${PROJECT_BINARY_DIR}/docs)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
set(DOXYGEN_CONFIG_FILE ${PROJECT_BINARY_DIR}/Doxyfile)

configure_file(${PROJECT_SOURCE_DIR}/Doxyfile.in ${DOXYGEN_CONFIG_FILE})

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG_FILE}
    MAIN_DEPENDENCY ${DOXYGEN_CONFIG_FILE}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating docs"
    VERBATIM
)
add_custom_target(docs DEPENDS ${DOXYGEN_INDEX_FILE})

message("Documentation will be generated")
add_dependencies(${PROJECT_NAME} docs)
