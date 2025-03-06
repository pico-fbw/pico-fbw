include(FetchContent)
FetchContent_Declare(
    mongoose
    GIT_REPOSITORY https://github.com/cesanta/mongoose
    GIT_TAG 7.17
)
FetchContent_MakeAvailable(mongoose)
