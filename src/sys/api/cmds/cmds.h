#pragma once

#include "platform/int.h"

/**
 * Handles API GET commands.
 * @param cmd the command
 * @param args the command arguments
 * @return the status code.
*/
i32 api_handle_get(const char *cmd, const char *args);

/**
 * Handles API SET commands.
 * @param cmd the command
 * @param args the command arguments
 * @return the status code.
*/
i32 api_handle_set(const char *cmd, const char *args);

/**
 * Handles API TEST commands.
 * @param cmd the command
 * @param args the command arguments
 * @return the status code.
*/
i32 api_handle_test(const char *cmd, const char *args);

/**
 * Handles API MISC (no prefix) commands.
 * @param cmd the command
 * @param args the command arguments
 * @return either status code 404, or -1 if the command was successful (MISC commands don't have return codes).
*/
i32 api_handle_misc(const char *cmd, const char *args);