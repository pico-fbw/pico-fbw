add_library(fbw_api
    api.c
    cmds/cmds.c
    cmds/GET/get_config.c
    cmds/GET/get_flightplan.c
    cmds/GET/get_info.c
    cmds/GET/get_input.c
    cmds/GET/get_logs.c
    cmds/GET/get_mode.c
    cmds/GET/get_sensor.c
    cmds/MISC/about.c
    cmds/MISC/help.c
    cmds/MISC/ping.c
    cmds/MISC/reboot.c
    cmds/MISC/reset.c
    cmds/SET/set_bay.c
    cmds/SET/set_config.c
    cmds/SET/set_flightplan.c
    cmds/SET/set_mode.c
    cmds/SET/set_target.c
    cmds/SET/set_waypoint.c
    cmds/TEST/test_all.c
    cmds/TEST/test_gps.c
    cmds/TEST/test_aahrs.c
    cmds/TEST/test_pwm.c
    cmds/TEST/test_servo.c
    cmds/TEST/test_throttle.c
)

target_link_libraries(fbw_api
    fbw_io
    fbw_lib
    fbw_modes
    fbw_sys
)

target_include_directories(fbw_api PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
