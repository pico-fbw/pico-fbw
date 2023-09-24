#ifndef __API_H
#define __API_H

// TODO: api documentation on wiki
// ensure to add a note in docs about how SET_MODE command can succeed but be booted out of the mode later

/**
 * Polls the API for new data (incoming commands) and responds if necessary.
*/
void api_poll();

#endif // __API_H
