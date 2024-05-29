#pragma once

#include "platform/defs.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <stdbool.h>
#include "esp_err.h"
#include "esp_http_server.h"

// clang-format on

/**
 * Starts the HTTP server.
 * @param server the server handle to store the server in
 * @return ESP_OK if successful, an ESP_ERR otherwise
*/
esp_err_t http_server_open(httpd_handle_t *server);

/**
 * Stops the HTTP server.
 * @param server the server handle to close
 * @return ESP_OK if successful, an ESP_ERR otherwise
*/
esp_err_t http_server_close(httpd_handle_t server);

#endif // PLATFORM_SUPPORTS_WIFI
