# Generate headers (to be included in fusion drivers) from binary firmware blobs
function(generate_firmware_headers firmware_files)
    set(FIRMWARE_BIN_DIR "${CMAKE_SOURCE_DIR}/lib/drivers/firmware")
    set(FIRMWARE_HEADERS_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include/firmware")
    file(MAKE_DIRECTORY ${FIRMWARE_HEADERS_INCLUDE_DIR})
    set(GENERATED_HEADERS)
    # For each firmware file, use xxd to generate a C header file into the build directory
    foreach(file ${firmware_files})
        set(HEADER_PATH "${CMAKE_BINARY_DIR}/include/firmware/${file}.h")
        add_custom_command(
            OUTPUT ${HEADER_PATH}
            WORKING_DIRECTORY ${FIRMWARE_BIN_DIR}
            COMMAND xxd -i ${file} > ${HEADER_PATH}
            DEPENDS ${FIRMWARE_BIN_DIR}/${file}
            COMMENT "Generating firmware header for ${file}"
            VERBATIM
        )
        list(APPEND GENERATED_HEADERS ${HEADER_PATH})
    endforeach()
    
    add_custom_target(generate_firmware_headers ALL DEPENDS ${GENERATED_HEADERS})
    add_dependencies(${PROJECT_NAME} generate_firmware_headers)
endfunction()
