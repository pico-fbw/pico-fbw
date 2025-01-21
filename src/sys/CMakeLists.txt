target_sources(${PROJECT_NAME} PRIVATE
    boot.c
    configuration.c
    control.c
    flightplan.c
    log.c
    runtime.c
    throttle.c
    version.c
)

add_subdirectory(api)
