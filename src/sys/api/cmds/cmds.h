#ifndef __CMDS_H
#define __CMDS_H

/**
 * Handles API GET commands.
 * @param cmd the command
 * @param args the command arguments
 * @return the status code.
*/
uint api_handle_get(const char *cmd, const char *args);

/**
 * Handles API SET commands.
 * @param cmd the command
 * @param args the command arguments
 * @return the status code.
*/
uint api_handle_set(const char *cmd, const char *args);

/**
 * Handles API TEST commands.
 * @param cmd the command
 * @param args the command arguments
 * @return the status code.
*/
uint api_handle_test(const char *cmd, const char *args);

/**
 * Handles API MISC (no prefix) commands.
 * @param cmd the command
 * @param args the command arguments
 * @return the status code.
*/
uint api_handle_misc(const char *cmd, const char *args);

#endif // __CMDS_H