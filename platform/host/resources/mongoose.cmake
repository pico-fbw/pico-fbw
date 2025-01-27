function(add_mongoose_to_target target)
    include(FetchContent)
    FetchContent_Declare(
        mongoose
        GIT_REPOSITORY https://github.com/cesanta/mongoose
        GIT_TAG 7.16
    )
    FetchContent_MakeAvailable(mongoose)
    target_sources(${target} PRIVATE ${mongoose_SOURCE_DIR}/mongoose.c)
    target_include_directories(${target} PRIVATE ${mongoose_SOURCE_DIR})
endfunction()
