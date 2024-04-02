#pragma once

/**
 * Polls the API for new data (incoming commands) and responds if necessary.
 * @return the status code of the executed command, or 0 if no command was executed
 */
i32 api_poll();

/**
 * Calls the API with the given command and arguments.
 * @param cmd the command to execute
 * @param args the arguments to pass to the command
 * @param result pointer to a buffer to store the result of the command
 * @note The caller is responsible for freeing the result buffer.
 * This function is not threadsafe.
*/
i32 api_call(const char *cmd, const char *args, char **result);
