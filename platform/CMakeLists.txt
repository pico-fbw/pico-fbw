# Will automatically link the selected platform to the rest of the project so it can be used
function(configure_libraries target_library)
    target_link_libraries(${PROJECT_NAME} ${target_library})
    target_link_libraries(fbw_api ${target_library})
    target_link_libraries(fbw_io ${target_library})
    target_link_libraries(fbw_lib ${target_library})
    target_link_libraries(fbw_modes ${target_library})
    target_link_libraries(fbw_sys ${target_library})
endfunction()

# Add the platform specific subdirectory to compile based on the platform variable
if (FBW_PLATFORM STREQUAL "pico" OR FBW_PLATFORM STREQUAL "pico_w")
    add_subdirectory(pico)
    configure_libraries(platform_pico)
elseif (FBW_PLATFORM STREQUAL "esp32")
    # idf_build_process(esp)
    configure_libraries(platform_esp)
endif()

# TODO: add "example" platform folder with comments explaining what people have to implement for each function
