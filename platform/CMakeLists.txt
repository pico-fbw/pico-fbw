# Will automatically link the selected platform to the rest of the project so it can be used
function(configure_libraries target_library)
    target_link_libraries(${PROJECT_NAME} ${target_library})
    target_link_libraries(fbw_api ${target_library})
    target_link_libraries(fbw_io ${target_library})
    target_link_libraries(fbw_lib ${target_library})
    target_link_libraries(fbw_modes ${target_library})
    target_link_libraries(fbw_sys ${target_library})
endfunction()

add_subdirectory(${PLATFORM_DIR})
message("Target platform library is: platform_${PLATFORM_DIR}")
configure_libraries(platform_${PLATFORM_DIR})
