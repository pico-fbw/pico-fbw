#ifndef __API_H
#define __API_H

#include "../config.h"

#ifdef API_ENABLED

// TODO: api documentation on docs

#define ENABLE_API_TIMEOUT_MS 1500 // Allow API calls this amount of time in ms after power on (prevents API calls during bootup)

/**
 * Initalizes the pico-fbw API.
 * Waits (blocks) for a response from the client before returning if API_WAIT_ON_BOOT is defined.
*/
void api_init_blocking();

/**
 * Polls the API for new data (incoming commands).
*/
void api_poll();

#endif // API_ENABLED

#endif // __API_H
