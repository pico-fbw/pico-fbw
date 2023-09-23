#ifndef __API_H
#define __API_H

#include "../config.h"

#ifdef API_ENABLED

// TODO: api documentation on wiki
// ensure to add a note in docs about how SET_MODE command can succeed but be booted out of the mode later

#define API_CHAR_TIMEOUT_US 100 // Timeout for waiting for a character to arrive

#define ENABLE_API_TIMEOUT_MS 3000 // Allow API calls this amount of time in ms after power on (prevents API calls during bootup)

/**
 * Polls the API for new data (incoming commands) and responds if necessary.
*/
void api_poll();

#endif // API_ENABLED

#endif // __API_H
