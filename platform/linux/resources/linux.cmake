# See platform/example/resources/example.cmake for comments regarding the structure of this file
add_compile_definitions(FBW_PLATFORM_LINUX=1)

set(LFS_BLOCK_SIZE 4096)
set(LFS_PROG_SIZE 1)
set(LFS_IMG_SIZE 262144) # 256KB

function(setup_before_subdirs)
    add_compile_options(-Wno-format)
    add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.c)
endfunction()

function(setup_after_subdirs)
    # Fetch, compile, and include lgpio to provide I/O support
    include(FetchContent)
    FetchContent_Declare(
        lgpio
        GIT_REPOSITORY https://github.com/joan2937/lg
        GIT_TAG v0.2.2
    )
    FetchContent_MakeAvailable(lgpio)
    target_sources(platform_linux PRIVATE
        ${lgpio_SOURCE_DIR}/lgCtx.c
        ${lgpio_SOURCE_DIR}/lgDbg.c
        ${lgpio_SOURCE_DIR}/lgErr.c
        ${lgpio_SOURCE_DIR}/lgGpio.c
        ${lgpio_SOURCE_DIR}/lgHdl.c
        ${lgpio_SOURCE_DIR}/lgI2C.c
        ${lgpio_SOURCE_DIR}/lgNotify.c
        ${lgpio_SOURCE_DIR}/lgPthAlerts.c
        ${lgpio_SOURCE_DIR}/lgPthTx.c
        ${lgpio_SOURCE_DIR}/lgSerial.c
        ${lgpio_SOURCE_DIR}/lgSPI.c
        ${lgpio_SOURCE_DIR}/lgThread.c
        ${lgpio_SOURCE_DIR}/lgUtil.c
    )
    target_include_directories(platform_linux PRIVATE ${lgpio_SOURCE_DIR})
    if (${FBW_BUILD_WWW})
        include(Mongoose)
        target_sources(platform_linux PRIVATE ${mongoose_SOURCE_DIR}/mongoose.c)
        target_include_directories(platform_linux PRIVATE ${mongoose_SOURCE_DIR})
        # Use flash.c provided by the host platform
        include_wwwfs(${CMAKE_SOURCE_DIR}/platform/host/flash.c)
    endif()
endfunction()
