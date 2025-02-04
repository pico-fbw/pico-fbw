target_sources(${PROJECT_NAME} PRIVATE
    boot.c
    configuration.c
    flightplan.c
    log.c
    runtime.c
    version.c
)

add_subdirectory(api)
