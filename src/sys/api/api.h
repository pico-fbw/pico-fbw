#ifndef __API_H
#define __API_H

/**
 * Polls the API for new data (incoming commands) and responds if necessary.
 * @return The status code of the executed command, or 0 if no command was executed
*/
int api_poll();

#endif // __API_H
